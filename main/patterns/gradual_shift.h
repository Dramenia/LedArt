#pragma once
#include "pattern.h"
#include "octagon/fiber.h"

struct GradualShiftParams {
    float   speed = 1.0f;
    uint8_t r_min = 0,   r_max = 255;
    uint8_t g_min = 0,   g_max = 255;
    uint8_t b_min = 0,   b_max = 255;
};

class GradualShift : public Pattern {
public:
    explicit GradualShift(const GradualShiftParams& params = {}) : params_(params) {}

    void setParams(const GradualShiftParams& params) { params_ = params; }
    const GradualShiftParams& getParams() const { return params_; }

    void begin(Octagon& octagon) override;
    void update(Octagon& octagon, uint32_t delta_ms) override;

private:
    GradualShiftParams params_;
    float phase_r_[16], phase_g_[16], phase_b_[16];
    float speed_r_[16], speed_g_[16], speed_b_[16];
};
