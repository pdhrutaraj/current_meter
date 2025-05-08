/* Industrial Current Meter (0-10A DC) - Single File Implementation */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "driver/i2c.h"
#include "esp_netif.h"

/* Configuration */
#define INA226_ADDR       0x40
#define I2C_MASTER_NUM    I2C_NUM_0
#define I2C_MASTER_FREQ   100000
#define AP_SSID          "CurrentMeter_AP"
#define AP_PASS          "measure123"
#define SHUNT_RESISTOR   0.1f    // 0.1Ω shunt
#define MAX_CURRENT      10.0f   // 10A max

/* Global Variables */
static const char *TAG = "CurrentMeter";
static QueueHandle_t current_queue;
static httpd_handle_t server = NULL;

/* Embedded Web Interface */
static const char WEB_PAGE[] = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"<title>Current Meter</title>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<style>"
"body {font-family: Arial; text-align: center; margin-top: 20px;}"
".current {font-size: 3em; color: #2c3e50; margin: 20px;}"
"</style>"
"</head>"
"<body>"
"<h1>Industrial Current Meter</h1>"
"<div class='current' id='current'>--.-- A</div>"
"<script>"
"function updateCurrent() {"
"  fetch('/current').then(r=>r.text()).then(t=>{"
"    document.getElementById('current').innerText = t + ' A';"
"    setTimeout(updateCurrent, 1000);"
"  });"
"}"
"updateCurrent();"
"</script>"
"</body>"
"</html>";

/* INA226 Driver Functions */
void i2c_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void ina226_write_reg(uint8_t reg, uint16_t value) {
    uint8_t data[3] = {reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write(cmd, data, sizeof(data), true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void ina226_init() {
    // Config: 16 samples avg, 1.1ms conversion time
    ina226_write_reg(0x00, 0x4527);
    // Calibration: 0.1Ω shunt, 10A max
    uint16_t cal = (uint16_t)(0.00512 / (SHUNT_RESISTOR * (MAX_CURRENT / 32768.0f)));
    ina226_write_reg(0x05, cal);
}

float ina226_read_current() {
    uint8_t reg = 0x04; // Current register
    uint8_t data[2];
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (INA226_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (INA226_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, sizeof(data), I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return (int16_t)((data[0] << 8) | data[1]) * (MAX_CURRENT / 32768.0f);
}

/* WiFi AP Setup */
void wifi_init_ap() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi AP Started. SSID: %s", AP_SSID);
}

/* HTTP Handlers */
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, WEB_PAGE, strlen(WEB_PAGE));
}

static esp_err_t current_get_handler(httpd_req_t *req) {
    float current;
    char current_str[16];
    
    if(xQueuePeek(current_queue, &current, 0)) {
        snprintf(current_str, sizeof(current_str), "%.3f", current);
    } else {
        strcpy(current_str, "--.--");
    }
    
    return httpd_resp_send(req, current_str, strlen(current_str));
}

void start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t current = {
            .uri = "/current",
            .method = HTTP_GET,
            .handler = current_get_handler
        };
        httpd_register_uri_handler(server, &current);
        
        ESP_LOGI(TAG, "Web server started");
    }
}

/* Main Tasks */
void current_task(void *pvParameters) {
    i2c_init();
    ina226_init();

    while (1) {
        float current = 0;
        // Oversampling: Take 16 samples and average
        for (int i = 0; i < 16; i++) {
            current += ina226_read_current();
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        current /= 16.0f;
        xQueueOverwrite(current_queue, &current);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // Create queue for current values
    current_queue = xQueueCreate(1, sizeof(float));
    
    // Start network
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    wifi_init_ap();
    
    // Start tasks
    xTaskCreate(current_task, "current_task", 4096, NULL, 5, NULL);
    start_webserver();
}