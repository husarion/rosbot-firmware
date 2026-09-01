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

#include "comm_backend.hpp"
#include "imu_interface.hpp"

// Persisted in STM32F407 flash sector 11 (0x080E0000, 128 KB). This isn't a
// carved-out MEMORY region in the linker script — it's enforced by capping
// `upload.maximum_size` at 917504 (0xE0000, the sector 10 boundary) in
// boards/rosbot_stm32f407.json, which shrinks the `FLASH` region the
// STM32duino core's own linker script generates from that value. A build
// that grows .text/.data past sector 10 fails at link time ("region FLASH
// overflowed"), so a reflash can never silently spill into and corrupt this
// sector — verified by temporarily setting maximum_size low enough to
// trigger the overflow.
// load() returns defaults (MAVLINK, empty namespace, no IMU calibration)
// on a fresh or corrupt sector. save() is a no-op when the cached value
// matches, so wear scales with config changes, not boots.
//
// Comm backend/namespace and BNO055 calibration offsets share one record
// in the same sector — a save() always writes the full Config, so callers
// must round-trip fields they don't intend to change (load() first).
//
// load() also recognizes the pre-IMU-calibration on-flash layout (see
// LegacyRecord in the .cpp) and migrates backend/namespace from it —
// an already-deployed unit's saved config isn't lost on this firmware's
// first boot. has_imu_calibration is always false from that path, which
// is correct: those units never had one.
namespace persistent_config {

inline constexpr size_t kNamespaceMaxLen = 32;

struct Config {
  CommBackend backend;
  char ns[kNamespaceMaxLen];
  bool has_imu_calibration = false;
  ImuCalibrationOffsets imu_calibration{};
};

Config load();

// Must run before vTaskStartScheduler(); the 1-3 s sector erase stalls
// the instruction bus and starves the motor watchdog / MAVLink TX DMA.
// Asserts if invoked once the scheduler is running.
void save(const Config& cfg);

}  // namespace persistent_config
