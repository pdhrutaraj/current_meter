#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ina226.h"
#include "esp_log.h"
#include "web_server.h"
#include "nvs_flash.h"

static const char *TAG = "CURRENT_METER";

float current_ma = 0;

void current_task(void *arg) {
    ina226_init(I2C_NUM_0, 0x40);
    while (1) {
        ina226_read_current(&current_ma);
        ESP_LOGI(TAG, "Current: %.2f mA", current_ma);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

float get_current_reading() {
    return current_ma;
}

void app_main(void) {
    nvs_flash_init();
    wifi_init_softap();
    xTaskCreate(current_task, "current_task", 2048, NULL, 5, NULL);
    start_webserver();
}
