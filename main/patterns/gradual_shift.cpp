#include "patterns/gradual_shift.h"
#include "octagon/octagon.h"
#include "esp_random.h"
#include <cmath>

static float randFloat() {
    return (float)(esp_random() % 10000) / 10000.0f;
}

// Returns a value oscillating between mn and mx using a sine wave.
static uint8_t sine(float phase, uint8_t mn, uint8_t mx) {
    float v = 0.5f + 0.5f * sinf(phase);
    return (uint8_t)(mn + (mx - mn) * v);
}

void GradualShift::begin(Octagon& /*octagon*/) {
    for (int i = 0; i < 16; i++) {
        // Random starting phases spread across the sine cycle.
        phase_r_[i] = randFloat() * 6.2832f;
        phase_g_[i] = randFloat() * 6.2832f;
        phase_b_[i] = randFloat() * 6.2832f;

        // One full cycle takes 5–20 seconds at speed = 1.0.
        speed_r_[i] = 0.000314f + randFloat() * 0.000943f; // rad/ms
        speed_g_[i] = 0.000314f + randFloat() * 0.000943f;
        speed_b_[i] = 0.000314f + randFloat() * 0.000943f;
    }
}

void GradualShift::update(Octagon& octagon, uint32_t delta_ms) {
    float dt = (float)delta_ms;
    for (int i = 0; i < 16; i++) {
        phase_r_[i] += speed_r_[i] * params_.speed * dt;
        phase_g_[i] += speed_g_[i] * params_.speed * dt;
        phase_b_[i] += speed_b_[i] * params_.speed * dt;

        octagon.fibers[i].color = {
            sine(phase_r_[i], params_.r_min, params_.r_max),
            sine(phase_g_[i], params_.g_min, params_.g_max),
            sine(phase_b_[i], params_.b_min, params_.b_max),
        };
    }
}
