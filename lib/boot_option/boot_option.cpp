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

#include "clock.hpp"
#include "time_base.hpp"

namespace {

bool anyButtonLow(const BootOptionConfig& cfg) {
  for (const auto& button : cfg.buttons) {
    if (!button.read()) return true;
  }
  return false;
}

void setGreen(const BootOptionConfig& cfg, bool on) {
  for (const auto& led : cfg.green_leds) led.write(on);
}

constexpr int kCalibrationConfirmBlinks = 3;
constexpr uint32_t kCalibrationConfirmBlinkMs = 150;

void confirmCalibrationEntry(const BootOptionConfig& cfg) {
  for (int i = 0; i < kCalibrationConfirmBlinks; ++i) {
    setGreen(cfg, true);
    platform::clock::delayMs(kCalibrationConfirmBlinkMs);
    setGreen(cfg, false);
    platform::clock::delayMs(kCalibrationConfirmBlinkMs);
  }
}

}  // namespace

BootAction resolveBootAction(const BootOptionConfig& cfg) {
  const uint32_t detect_start = time_boot_ms();
  while (!anyButtonLow(cfg)) {
    if (time_boot_ms() - detect_start >= cfg.press_detect_window_ms) {
      return BootAction::kNone;
    }
    platform::clock::delayMs(10);
  }

  const uint32_t start = time_boot_ms();
  while (anyButtonLow(cfg)) {
    if (time_boot_ms() - start >= cfg.calibration_hold_ms) {
      confirmCalibrationEntry(cfg);
      return BootAction::kCalibrateImu;
    }
    platform::clock::delayMs(10);
  }

  setGreen(cfg, true);
  return BootAction::kChangeTransport;
}
