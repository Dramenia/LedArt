#include "octagon.h"
#include "patterns/pattern.h"
#include "esp_log.h"

static const char* TAG = "Octagon";

Octagon::Octagon(const OctagonConfig& config) : pattern_(nullptr) {
    for (int i = 0; i < 16; i++) {
        fibers[i].led_a  = config.fibers[i].led_a;
        fibers[i].led_b  = config.fibers[i].led_b;
        fibers[i].color  = {0, 0, 0};
    }

    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num          = config.gpio_pin;
    strip_config.max_leds                = 32;
    strip_config.led_model               = LED_MODEL_WS2812;
    strip_config.color_component_format  = LED_STRIP_COLOR_COMPONENT_FMT_GRB;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz     = 10 * 1000 * 1000; // 10 MHz
    rmt_config.mem_block_symbols = 512; // max ESP32 RMT memory — minimises interrupt refills

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &strip_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init LED strip on GPIO %d: %s", config.gpio_pin, esp_err_to_name(err));
    }
}

Octagon::~Octagon() {
    delete pattern_;
    led_strip_del(strip_);
}

void Octagon::setPattern(Pattern* pattern) {
    delete pattern_;
    pattern_ = pattern;
    if (pattern_) {
        pattern_->begin(*this);
    }
}

void Octagon::update(uint32_t delta_ms) {
    if (pattern_) {
        pattern_->update(*this, delta_ms);
    }
}

void Octagon::render() {
    for (int i = 0; i < 16; i++) {
        const Color& c = fibers[i].color;
        led_strip_set_pixel(strip_, fibers[i].led_a, c.r, c.g, c.b);
        led_strip_set_pixel(strip_, fibers[i].led_b, c.r, c.g, c.b);
    }
    led_strip_refresh(strip_);
}
