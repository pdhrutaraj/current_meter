
#include "ina226.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define INA226_CALIB_REG 0x05
#define INA226_CURRENT_REG 0x04

static const char *TAG = "INA226";

esp_err_t ina226_write_register(i2c_port_t i2c_num, uint8_t addr, uint8_t reg, uint16_t value) {
    uint8_t data[3] = { reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    return i2c_master_write_to_device(i2c_num, addr, data, 3, 1000 / portTICK_PERIOD_MS);
}

esp_err_t ina226_read_register(i2c_port_t i2c_num, uint8_t addr, uint8_t reg, uint16_t *value) {
    uint8_t data[2];
    esp_err_t err = i2c_master_write_read_device(i2c_num, addr, &reg, 1, data, 2, 1000 / portTICK_PERIOD_MS);
    if (err == ESP_OK) {
        *value = ((uint16_t)data[0] << 8) | data[1];
    }
    return err;
}

esp_err_t ina226_init(i2c_port_t i2c_num, uint8_t addr) {
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(ina226_write_register(i2c_num, addr, INA226_CALIB_REG, 2048));
    return ESP_OK;
}

float ina226_read_current_mA(i2c_port_t i2c_num, uint8_t addr) {
    uint16_t raw;
    if (ina226_read_register(i2c_num, addr, INA226_CURRENT_REG, &raw) == ESP_OK) {
        int16_t signed_raw = (int16_t)raw;
        return signed_raw * 0.1f;
    }
    return 0.0f;
}
