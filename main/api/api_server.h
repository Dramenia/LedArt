#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "patterns/gradual_shift.h"
#include "patterns/solid_color.h"

enum class PatternId : uint8_t { GRADUAL_SHIFT, SOLID_COLOR };

struct ApiCommand {
    PatternId          pattern = PatternId::GRADUAL_SHIFT;
    GradualShiftParams gradual;
    Color              solid;
};

class ApiServer {
public:
    static void start(QueueHandle_t cmd_queue);
    static void stop();
};
