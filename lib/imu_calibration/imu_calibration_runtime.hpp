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

#include <atomic>
#include <cstdint>

#include "imu_interface.hpp"

// Runtime IMU calibration: the BNO055 calibrates continuously in its fusion
// mode, so there is nothing to "start" — the host watches the live level
// (ROSBOT_IMU_CALIBRATION) while the robot is moved, then asks for the
// offsets to be persisted (MAV_CMD_USER_2). The request arrives on the
// MAVLink task but the bus and the offsets belong to the IMU task, so the
// two meet only through this struct.
namespace imu_calibration {

enum class SaveState : uint8_t {
  kNone = 0,
  kSaving = 1,
  kSaved = 2,
  kRejected = 3,  // not fully calibrated when the save ran
  kFailed = 4,    // I2C or flash error
};

enum class Action : uint8_t {
  kStop = 0,
  kStart = 1,
  kSave = 2,
};

// A session exists only to give the operator feedback on the robot itself
// (red LED blinking fast) — the chip calibrates with or without one. It
// ends on a successful save, a stop, or when it expires, so a host that
// dies mid-session doesn't leave the LED blinking forever.
inline constexpr uint32_t kSessionTimeoutMs = 180000;

struct Control {
  std::atomic<bool> save_requested{false};
  std::atomic<uint8_t> state{static_cast<uint8_t>(SaveState::kNone)};
  std::atomic<uint8_t> save_seq{0};
  std::atomic<bool> session_active{false};
  std::atomic<uint32_t> session_started_ms{0};
};

inline Control g_control;

// Called from the IMU task, between update()s: reads the offsets the chip
// is using and appends them to flash. Publishes the outcome in g_control.
void serviceSave(ImuInterface& imu);

void startSession(uint32_t now_ms);
void stopSession();
// Also retires an expired session, so callers need no timer of their own.
bool sessionActive(uint32_t now_ms);

}  // namespace imu_calibration
