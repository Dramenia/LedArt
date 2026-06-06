#pragma once
#include <cstdint>

class Octagon;

class Pattern {
public:
    virtual void begin(Octagon& octagon) = 0;
    virtual void update(Octagon& octagon, uint32_t delta_ms) = 0;
    virtual ~Pattern() = default;
};
