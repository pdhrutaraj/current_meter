#include "ina226.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define INA226_REG_CURRENT 0x04
#define INA226_REG_CALIBRATION 0x05

static i2c_port_t g_i2c_port = I2C_NUM_0;
static uint8_t g_device_addr = 0x40;

esp_err_t ina226_init(i2c_port_t i2c_num, uint8_t addr) {
    g_i2c_port = i2c_num;
    g_device_addr = addr;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(g_i2c_port, &conf);
    i2c_driver_install(g_i2c_port, conf.mode, 0, 0, 0);

    return ESP_OK;
}

esp_err_t ina226_read_current(float *current_ma) {
    uint8_t data[2];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (g_device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, INA226_REG_CURRENT, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (g_device_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(g_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    int16_t raw_current = (data[0] << 8) | data[1];
    *current_ma = raw_current * 0.1; // adjust this scale according to calibration
    return ESP_OK;
}