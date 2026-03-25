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

#include <Arduino.h>
#include <std_msgs/msg/u_int8.h>

#include "../types.hpp"

struct LedConfig {
  uint8_t pin;
  uint8_t bit_mask;
};

struct LedState {
  std_msgs__msg__UInt8 msg = {};
  const LedConfig* config;
  size_t config_count;
};

inline void ledsCallback(const void* msg_in) {
  auto* state = reinterpret_cast<const LedState*>(msg_in);
  uint8_t led_states = state->msg.data;

  for (size_t i = 0; i < state->config_count; i++) {
    const LedConfig& cfg = state->config[i];
    bool is_on = (led_states & cfg.bit_mask) != 0;
    digitalWrite(cfg.pin, is_on ? HIGH : LOW);
  }
}
