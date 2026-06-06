#include "wifi/wifi_manager.h"
#include "wifi/config_server.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs.h"
#include "lwip/ip4_addr.h"
#include <cstring>

static const char* TAG       = "WiFiManager";
static const char  AP_SSID[] = "LedArt-Setup";

void WiFiManager::begin() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    char ssid[CRED_LEN] = {};
    char pass[CRED_LEN] = {};

    if (loadCredentials(ssid, pass)) {
        ESP_LOGI(TAG, "Credentials found, connecting to '%s'", ssid);
        mode_ = Mode::STA;
        startSTA(ssid, pass);
    } else {
        ESP_LOGI(TAG, "No credentials — starting hotspot '%s'", AP_SSID);
        mode_ = Mode::AP;
        startAP();
        ConfigServer::start();
    }
}

bool WiFiManager::loadCredentials(char* ssid, char* password) {
    nvs_handle_t nvs;
    if (nvs_open("wifi", NVS_READONLY, &nvs) != ESP_OK) return false;

    size_t ssid_len = CRED_LEN;
    size_t pass_len = CRED_LEN;
    bool ok = (nvs_get_str(nvs, "ssid", ssid, &ssid_len) == ESP_OK) &&
              (nvs_get_str(nvs, "pass", password, &pass_len) == ESP_OK) &&
              ssid[0] != '\0';
    nvs_close(nvs);
    return ok;
}

void WiFiManager::startAP() {
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_config = {};
    memcpy(ap_config.ap.ssid, AP_SSID, sizeof(AP_SSID));
    ap_config.ap.ssid_len       = strlen(AP_SSID);
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode       = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Hotspot ready — connect to '%s' and open 192.168.4.1", AP_SSID);
}

void WiFiManager::startSTA(const char* ssid, const char* password) {
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiManager::eventHandler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFiManager::eventHandler, this, nullptr));

    wifi_config_t sta_config = {};
    strncpy((char*)sta_config.sta.ssid,     ssid,     sizeof(sta_config.sta.ssid)     - 1);
    strncpy((char*)sta_config.sta.password, password, sizeof(sta_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void WiFiManager::eventHandler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    WiFiManager* self = static_cast<WiFiManager*>(arg);

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (self->retries_ < MAX_RETRIES) {
            self->retries_++;
            ESP_LOGW(TAG, "Disconnected — retrying (%d/%d)...", self->retries_, MAX_RETRIES);
            esp_wifi_connect();
        } else {
            self->connected_ = false;
            ESP_LOGE(TAG, "Could not connect after %d attempts", MAX_RETRIES);
        }

    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(data);
        ESP_LOGI(TAG, "Connected — IP: " IPSTR, IP2STR(&event->ip_info.ip));
        self->connected_ = true;
        self->retries_   = 0;
    }
}
