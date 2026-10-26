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

#include "led_indicator.hpp"

void LedIndicator::init() {
  pinMode(cfg_.pin, OUTPUT);
  digitalWrite(cfg_.pin, cfg_.initial_state);
}

void LedIndicator::calibrating(bool fully_calibrated) {
  constexpr uint32_t kFastBlinkMs = 100;
  resetSOS();
  if (fully_calibrated) {
    digitalWrite(cfg_.pin, LOW);
    return;
  }
  const uint32_t now = millis();
  if (now - last_toggle_ >= kFastBlinkMs) {
    digitalToggle(cfg_.pin);
    last_toggle_ = now;
  }
}

void LedIndicator::update(bool battery_low, bool link_disconnected,
                          bool error) {
  uint32_t now = millis();

  // Priority 1: error → SOS pattern
  if (error) {
    handleSOS(now);
    return;
  }
  resetSOS();

  // Priority 2: battery low → blink
  if (battery_low) {
    handleBlink(now);
    return;
  }

  // Priority 3: µROS disconnected → solid ON
  if (link_disconnected) {
    digitalWrite(cfg_.pin, HIGH);
    return;
  }

  // All OK → OFF
  digitalWrite(cfg_.pin, LOW);
}

void LedIndicator::handleBlink(uint32_t now) {
  if (now - last_toggle_ >= cfg_.blink_period_ms) {
    digitalToggle(cfg_.pin);
    last_toggle_ = now;
  }
}

void LedIndicator::handleSOS(uint32_t now) {
  // Morse SOS: dot 200, dash 600, 200 between marks, 600 between letters,
  // 1400 before repeating. Explicit levels, so the phase cannot drift.
  struct Step {
    bool on;
    uint16_t ms;
  };
  static constexpr Step kSos[] = {
      {true, 200},  {false, 200}, {true, 200},  {false, 200}, {true, 200},
      {false, 600}, {true, 600},  {false, 200}, {true, 600},  {false, 200},
      {true, 600},  {false, 600}, {true, 200},  {false, 200}, {true, 200},
      {false, 200}, {true, 200},  {false, 1400}};
  constexpr uint8_t kSteps = sizeof(kSos) / sizeof(kSos[0]);

  if (sos_step_ == kSosIdle) {
    sos_step_ = 0;
    last_toggle_ = now;
  } else if (now - last_toggle_ >= kSos[sos_step_].ms) {
    sos_step_ = (sos_step_ + 1) % kSteps;
    last_toggle_ = now;
  }
  digitalWrite(cfg_.pin, kSos[sos_step_].on ? HIGH : LOW);
}
