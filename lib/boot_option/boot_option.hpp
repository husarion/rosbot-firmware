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

#pragma once

#include <cstddef>
#include <cstdint>

// Single point of truth for "what does the operator want at power-on".
// Replaces the old useAlt()/confirmAlt() button-polling pair that used to
// live in each variant's main.cpp: reads the buttons exactly once, blocks
// only as long as the gesture takes to resolve, and owns the LED
// confirmation for its own decision. Room to grow (e.g. a double-press
// gesture) without touching main.cpp again.
enum class BootAction {
  kNone,             // no button held at boot — normal start
  kChangeTransport,  // quick tap-and-release — select the diagnostic serial
  kCalibrateImu,     // held for cfg.calibration_hold_ms — enter the IMU
                     // calibration window (see imu_calibration_boot::run())
};

struct BootOptionConfig {
  // One or more interchangeable buttons — rosbot has two (either one
  // qualifies), rosbot_xl has one (PUSH_BUTTON2 there is wired to MCU
  // NRST, not a readable GPIO).
  const uint8_t* buttons;
  uint8_t button_count;

  // Green status LED(s) used to confirm the decision. Pass 0 for
  // green_led2 on boards with only one (rosbot_xl) — rosbot has two and
  // this lights both together, so both robots read identically.
  uint8_t green_led;
  uint8_t green_led2 = 0;

  uint32_t calibration_hold_ms = 3000;

  // How long to wait for a press to *start* before giving up on it.
  // Covers the operator who presses the button just before/around reset
  // rather than holding it continuously through the reset edge — without
  // this window, a press that starts a few hundred ms after
  // boardPheripheralsInit() would be missed entirely (checked only once,
  // at t=0).
  uint32_t press_detect_window_ms = 1500;
};

// Blocks only as long as the gesture itself takes:
//  - no button pressed within press_detect_window_ms of entry -> kNone
//  - pressed (any time within that window) and released before the hold
//    threshold                                   -> kChangeTransport;
//    green LED(s) latch on solid immediately
//  - held past calibration_hold_ms               -> kCalibrateImu, decided
//    the instant the threshold is crossed (does not wait for release);
//    green LED(s) blink 3x to confirm, then turn off. The calibration
//    wait itself (red blink / green solid on success) is owned by
//    imu_calibration_boot::run(), not this function.
BootAction resolveBootAction(const BootOptionConfig& cfg);
