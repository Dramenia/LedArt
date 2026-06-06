#pragma once
#include "esp_event.h"

class WiFiManager {
public:
    enum class Mode { AP, STA };

    void begin();
    bool isConnected() const { return connected_; }
    Mode getMode()     const { return mode_; }

private:
    void startAP();
    void startSTA(const char* ssid, const char* password);
    static bool loadCredentials(char* ssid, char* password);
    static void eventHandler(void* arg, esp_event_base_t base, int32_t id, void* data);

    bool    connected_ = false;
    uint8_t retries_   = 0;
    Mode    mode_      = Mode::AP;

    static constexpr uint8_t MAX_RETRIES = 5;
    static constexpr size_t  CRED_LEN    = 64;
};
