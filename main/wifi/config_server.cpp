#include "wifi/config_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <cctype>
#include <cstdlib>

static const char* TAG    = "ConfigServer";
static httpd_handle_t srv = nullptr;

static const char CONFIG_HTML[] = R"html(<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LedArt Setup</title>
<style>
body{font-family:sans-serif;max-width:380px;margin:40px auto;padding:20px}
h2{color:#e74c3c}
label{display:block;margin-top:14px;font-size:14px;color:#555}
input{width:100%;padding:10px;margin-top:4px;box-sizing:border-box;border:1px solid #ccc;border-radius:4px;font-size:15px}
button{width:100%;padding:13px;margin-top:22px;background:#e74c3c;color:#fff;border:none;border-radius:4px;font-size:16px;cursor:pointer}
</style>
</head>
<body>
<h2>LedArt WiFi Setup</h2>
<form method="POST" action="/connect">
<label>Network name (SSID)</label>
<input type="text" name="ssid" autocomplete="off" required>
<label>Password</label>
<input type="password" name="password" autocomplete="off">
<button type="submit">Connect</button>
</form>
</body>
</html>)html";

static const char SUCCESS_HTML[] = R"html(<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LedArt</title>
<style>body{font-family:sans-serif;max-width:380px;margin:40px auto;padding:20px}h2{color:#27ae60}</style>
</head>
<body>
<h2>Credentials saved</h2>
<p>LedArt is rebooting and will connect to your network.</p>
</body>
</html>)html";

static void urlDecode(const char* src, char* dst, size_t maxlen) {
    size_t i = 0;
    while (*src && i < maxlen - 1) {
        if (*src == '+') {
            dst[i++] = ' ';
            src++;
        } else if (*src == '%' && isxdigit((uint8_t)src[1]) && isxdigit((uint8_t)src[2])) {
            char hex[3] = {src[1], src[2], '\0'};
            dst[i++] = (char)strtol(hex, nullptr, 16);
            src += 3;
        } else {
            dst[i++] = *src++;
        }
    }
    dst[i] = '\0';
}

static bool extractField(const char* body, const char* key, char* out, size_t maxlen) {
    const char* pos = strstr(body, key);
    if (!pos) return false;
    pos += strlen(key);
    char raw[128] = {};
    size_t i = 0;
    while (*pos && *pos != '&' && i < sizeof(raw) - 1) raw[i++] = *pos++;
    urlDecode(raw, out, maxlen);
    return true;
}

static esp_err_t handleGet(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, CONFIG_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t handleConnect(httpd_req_t* req) {
    char body[256] = {};
    int  len       = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0) { httpd_resp_send_500(req); return ESP_FAIL; }

    char ssid[64] = {};
    char pass[64] = {};
    if (!extractField(body, "ssid=", ssid, sizeof(ssid)) || ssid[0] == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing SSID");
        return ESP_FAIL;
    }
    extractField(body, "password=", pass, sizeof(pass));

    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open("wifi", NVS_READWRITE, &nvs));
    ESP_ERROR_CHECK(nvs_set_str(nvs, "ssid", ssid));
    ESP_ERROR_CHECK(nvs_set_str(nvs, "pass", pass));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);

    ESP_LOGI(TAG, "Saved credentials for '%s', rebooting...", ssid);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, SUCCESS_HTML, HTTPD_RESP_USE_STRLEN);

    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();
    return ESP_OK;
}

void ConfigServer::start() {
    httpd_config_t config  = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&srv, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }
    httpd_uri_t getUri  = {"/",       HTTP_GET,  handleGet,     nullptr};
    httpd_uri_t postUri = {"/connect", HTTP_POST, handleConnect, nullptr};
    httpd_register_uri_handler(srv, &getUri);
    httpd_register_uri_handler(srv, &postUri);
    ESP_LOGI(TAG, "Config server started — open http://192.168.4.1");
}

void ConfigServer::stop() {
    if (srv) { httpd_stop(srv); srv = nullptr; }
}
