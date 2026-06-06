#pragma once
#include "pattern.h"
#include "octagon/fiber.h"

class SolidColor : public Pattern {
public:
    explicit SolidColor(Color color = {0, 0, 0}) : color_(color) {}

    void setColor(Color color);

    void begin(Octagon& octagon) override;
    void update(Octagon& octagon, uint32_t delta_ms) override;

private:
    Color   color_;
    Octagon* octagon_ = nullptr;
};
