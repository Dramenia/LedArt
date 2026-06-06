#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "octagon/octagon.h"
#include "octagon/octagon_config.h"
#include "patterns/gradual_shift.h"
#include "patterns/solid_color.h"
#include "wifi/wifi_manager.h"
#include "api/api_server.h"

static const OctagonConfig OCTAGON_1 = {
    .gpio_pin = 17,
    .fibers = {
        {0,  29}, {1,  18}, {2,  28}, {3,  27},
        {4,  30}, {5,  6},  {7,  20},
        {8,  31}, {9,  11}, {10, 22},
        {12, 19}, {13, 15}, {14, 21}, {16, 26},
        {17, 24}, {23, 25},
    }
};

static void applyCommand(Octagon& octagon, PatternId& current_id, const ApiCommand& cmd) {
    if (cmd.pattern == PatternId::GRADUAL_SHIFT) {
        if (current_id == PatternId::GRADUAL_SHIFT) {
            // Same pattern — update params without resetting fiber phases.
            static_cast<GradualShift*>(octagon.currentPattern())->setParams(cmd.gradual);
        } else {
            octagon.setPattern(new GradualShift(cmd.gradual));
        }
    } else if (cmd.pattern == PatternId::SOLID_COLOR) {
        if (current_id == PatternId::SOLID_COLOR) {
            static_cast<SolidColor*>(octagon.currentPattern())->setColor(cmd.solid);
        } else {
            octagon.setPattern(new SolidColor(cmd.solid));
        }
    }
    current_id = cmd.pattern;
}

static void ledTask(void* arg) {
    auto* queue = static_cast<QueueHandle_t>(arg);

    Octagon  octagon(OCTAGON_1);
    PatternId current_id = PatternId::GRADUAL_SHIFT;
    octagon.setPattern(new GradualShift());

    int64_t last_time = esp_timer_get_time();

    while (true) {
        ApiCommand cmd;
        if (xQueueReceive(queue, &cmd, 0) == pdTRUE) {
            applyCommand(octagon, current_id, cmd);
        }

        int64_t  now      = esp_timer_get_time();
        uint32_t delta_ms = (uint32_t)((now - last_time) / 1000);
        last_time = now;
        if (delta_ms > 50) delta_ms = 50;

        octagon.update(delta_ms);
        octagon.render();

        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

extern "C" void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    WiFiManager* wifi = new WiFiManager();
    wifi->begin();

    if (wifi->getMode() == WiFiManager::Mode::STA) {
        QueueHandle_t cmd_queue = xQueueCreate(1, sizeof(ApiCommand));
        ApiServer::start(cmd_queue);
        xTaskCreatePinnedToCore(ledTask, "led", 4096, cmd_queue, 5, nullptr, 1);
    } else {
        // AP mode: just run LEDs, no API server.
        xTaskCreatePinnedToCore(ledTask, "led", 4096, xQueueCreate(1, sizeof(ApiCommand)), 5, nullptr, 1);
    }

    vTaskDelete(nullptr);
}
