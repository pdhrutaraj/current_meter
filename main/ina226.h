#pragma once

#include "driver/i2c.h"
#include "esp_err.h"
#include <stdint.h>

esp_err_t ina226_init(i2c_port_t i2c_num, uint8_t addr);
esp_err_t ina226_read_current(float *current_ma);