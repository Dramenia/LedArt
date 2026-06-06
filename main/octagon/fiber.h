#pragma once
#include <cstdint>

struct Color {
    uint8_t r, g, b;
};

struct Fiber {
    uint8_t led_a;
    uint8_t led_b;
    Color   color;
};
