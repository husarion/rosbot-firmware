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

/// Idle animation: expand from center outward
inline void idleAnimation(LedStrip& strip, uint8_t r, uint8_t g, uint8_t b,
                          TickType_t interval_ms, bool reset = false, uint8_t fade_steps = 4) {
  uint8_t half = strip.size() / 2;

  static uint8_t prev_r = 0, prev_g = 0, prev_b = 0;
  if(reset) {
    prev_r = prev_g = prev_b = 0;
  }

  for (uint8_t i = 0; i < half; ++i) {
    for (uint8_t step = 1; step <= fade_steps; ++step) {
      float k = (float)step / fade_steps;
      float inv_k = 1.0f - k;

      uint8_t rf = prev_r * inv_k + r * k;
      uint8_t gf = prev_g * inv_k + g * k;
      uint8_t bf = prev_b * inv_k + b * k;

      strip.setBuffer(half - 1 - i, rf, gf, bf);
      strip.setBuffer(half + i, rf, gf, bf);
      strip.show();
      vTaskDelay(interval_ms);
    }
  }

  prev_r = r;
  prev_g = g;
  prev_b = b;
}
