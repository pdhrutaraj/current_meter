
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "ina226.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

static const char *TAG = "CURRENT_METER";
static float current_mA = 0.0;

esp_err_t init_i2c() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

esp_err_t root_get_handler(httpd_req_t *req) {
    char html[256];
    snprintf(html, sizeof(html),
             "<!DOCTYPE html><html><head><title>Current Monitor</title></head><body>"
             "<h2>Current: %.2f mA</h2><script>"
             "setInterval(()=>fetch('/current').then(r=>r.text()).then(d=>document.querySelector('h2').innerText='Current: '+d+' mA'),500);"
             "</script></body></html>", current_mA);
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t current_get_handler(httpd_req_t *req) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f", current_mA);
    httpd_resp_send(req, buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {.uri="/", .method=HTTP_GET, .handler=root_get_handler};
        httpd_uri_t current = {.uri="/current", .method=HTTP_GET, .handler=current_get_handler};
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &current);
    }
    return server;
}

void wifi_init_softap() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "CurrentMonitor",
            .ssid_len = 0,
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        }
    };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();
}

void current_task(void *pvParameters) {
    while (1) {
        current_mA = ina226_read_current_mA(I2C_MASTER_NUM, 0x40);
        ESP_LOGI(TAG, "Current: %.2f mA", current_mA);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    nvs_flash_init();
    init_i2c();
    ina226_init(I2C_MASTER_NUM, 0x40);
    wifi_init_softap();
    start_webserver();
    xTaskCreate(current_task, "current_task", 4096, NULL, 5, NULL);
}
