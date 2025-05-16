
#pragma once
#include "driver/i2c.h"
#include "esp_err.h"

esp_err_t ina226_init(i2c_port_t i2c_num, uint8_t addr);
float ina226_read_current_mA(i2c_port_t i2c_num, uint8_t addr);
