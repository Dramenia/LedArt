#pragma once
#include <cstdint>

struct FiberConfig {
    uint8_t led_a;
    uint8_t led_b;
};

struct OctagonConfig {
    int          gpio_pin;
    FiberConfig  fibers[16];
};
