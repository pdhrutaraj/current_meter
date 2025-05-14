#pragma once
#include "esp_http_server.h"

httpd_handle_t start_webserver(void);
void wifi_init_softap(void);
float get_current_reading(void);