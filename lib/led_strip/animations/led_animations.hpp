#pragma once

#include "led_strip.hpp"

/// Idle animation: expand from center outward
inline void idleAnimation(
    LedStrip& strip, uint8_t r, uint8_t g, uint8_t b, TickType_t interval_ms)
{
    uint8_t len = strip.size();
    uint8_t half = len / 2;

    for (uint8_t i = 0; i < half; ++i) {
        strip.setBuffer(half - 1 - i, r, g, b);
        strip.setBuffer(half + i,     r, g, b);
        strip.show();
        vTaskDelay(interval_ms);
    }
}
