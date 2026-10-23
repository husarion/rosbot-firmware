// Copyright 2022 Husarion sp. z o.o.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "led_strip.hpp"

/// Idle animation: expand from center outward, fading from the colour of
/// the previous run.
class IdleAnimation {
 public:
  void run(LedStrip& strip, uint8_t r, uint8_t g, uint8_t b,
           TickType_t interval_ms, bool reset = false, uint8_t fade_steps = 3) {
    TickType_t wake_time = xTaskGetTickCount();
    const uint16_t half = strip.size() / 2;

    if (reset) {
      prev_r_ = prev_g_ = prev_b_ = 0;
      strip.clear();
      strip.show();
      vTaskDelayUntil(&wake_time, interval_ms);
    }

    for (uint8_t i = 0; i < half; ++i) {
      for (uint8_t step = 1; step <= fade_steps; ++step) {
        uint8_t rf =
            prev_r_ + (static_cast<int16_t>(r) - prev_r_) * step / fade_steps;
        uint8_t gf =
            prev_g_ + (static_cast<int16_t>(g) - prev_g_) * step / fade_steps;
        uint8_t bf =
            prev_b_ + (static_cast<int16_t>(b) - prev_b_) * step / fade_steps;

        strip.setBuffer(half - 1 - i, rf, gf, bf);
        strip.setBuffer(half + i, rf, gf, bf);
        strip.show();
        vTaskDelayUntil(&wake_time, interval_ms);
      }
      if (i == 3) vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(500));
    }

    prev_r_ = r;
    prev_g_ = g;
    prev_b_ = b;
  }

 private:
  uint16_t prev_r_ = 0;
  uint16_t prev_g_ = 0;
  uint16_t prev_b_ = 0;
};
