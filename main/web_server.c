#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "web_server.h"
#include "ina226.h"

static const char *HTML_PAGE = "<!DOCTYPE html><html><head><title>Current Monitor</title><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:sans-serif;text-align:center;padding-top:30px;background:#111;color:#fff}.card{background:#222;border-radius:10px;padding:20px;display:inline-block;box-shadow:0 0 10px #000}.value{font-size:48px;color:#4caf50}</style></head><body><div class='card'><h2>DC Current Monitor</h2><div class='value' id='current'>0.00 mA</div></div><script>setInterval(()=>{fetch('/current').then(r=>r.text()).then(c=>{document.getElementById('current').innerText=c+' mA'})}, 500);</script></body></html>";

esp_err_t html_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, HTML_PAGE, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t current_get_handler(httpd_req_t *req) {
    char current_str[16];
    snprintf(current_str, sizeof(current_str), "%.2f", get_current_reading());
    httpd_resp_send(req, current_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    httpd_start(&server, &config);

    httpd_uri_t html_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = html_get_handler,
    };

    httpd_uri_t current_uri = {
        .uri = "/current",
        .method = HTTP_GET,
        .handler = current_get_handler,
    };

    httpd_register_uri_handler(server, &html_uri);
    httpd_register_uri_handler(server, &current_uri);
    return server;
}

void wifi_init_softap(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "CurrentMeter",
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();
}
