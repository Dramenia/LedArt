#pragma once
#include "fiber.h"
#include "octagon_config.h"
#include "led_strip.h"
#include "led_strip_rmt.h"

class Pattern;

class Octagon {
public:
    explicit Octagon(const OctagonConfig& config);
    ~Octagon();

    // Takes ownership of pattern; deletes previous pattern immediately (hard cut).
    void     setPattern(Pattern* pattern);
    Pattern* currentPattern() const { return pattern_; }

    void update(uint32_t delta_ms);
    void render();

    Fiber fibers[16];

private:
    led_strip_handle_t strip_;
    Pattern*           pattern_;
};
