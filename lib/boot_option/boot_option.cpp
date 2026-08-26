// Copyright 2026 Husarion sp. z o.o.
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

#include "boot_option.hpp"

#include <Arduino.h>

namespace {

bool anyButtonLow(const BootOptionConfig& cfg) {
  for (uint8_t i = 0; i < cfg.button_count; ++i) {
    if (digitalRead(cfg.buttons[i]) == LOW) return true;
  }
  return false;
}

void setGreen(const BootOptionConfig& cfg, bool on) {
  digitalWrite(cfg.green_led, on ? HIGH : LOW);
  if (cfg.green_led2 != 0) digitalWrite(cfg.green_led2, on ? HIGH : LOW);
}

constexpr int kCalibrationConfirmBlinks = 3;
constexpr uint32_t kCalibrationConfirmBlinkMs = 150;

void confirmCalibrationEntry(const BootOptionConfig& cfg) {
  for (int i = 0; i < kCalibrationConfirmBlinks; ++i) {
    setGreen(cfg, true);
    delay(kCalibrationConfirmBlinkMs);
    setGreen(cfg, false);
    delay(kCalibrationConfirmBlinkMs);
  }
}

}  // namespace

BootAction resolveBootAction(const BootOptionConfig& cfg) {
  const uint32_t detect_start = millis();
  while (!anyButtonLow(cfg)) {
    if (millis() - detect_start >= cfg.press_detect_window_ms) {
      return BootAction::kNone;
    }
    delay(10);
  }

  const uint32_t start = millis();
  while (anyButtonLow(cfg)) {
    if (millis() - start >= cfg.calibration_hold_ms) {
      confirmCalibrationEntry(cfg);
      return BootAction::kCalibrateImu;
    }
    delay(10);
  }

  setGreen(cfg, true);
  return BootAction::kChangeTransport;
}
