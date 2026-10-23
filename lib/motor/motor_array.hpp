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

#include <atomic>

#include "STM32FreeRTOS.h"
#include "motor_interface.hpp"

#define MAX_NUM_MOTORS 4

/// H-bridge driver group (e.g. one DRV8848 chip → 2 channels).
struct DriverGroupConfig {
  uint8_t sleep_pin;  // nSLEEP output
  uint8_t fault_pin;  // nFAULT input (active-LOW)
};

/// Aggregate data snapshot for all motors.
struct MotorsData {
  float position[MAX_NUM_MOTORS] = {};
  float velocity[MAX_NUM_MOTORS] = {};
  float effort[MAX_NUM_MOTORS] = {};
  uint8_t count = 0;
};

/// Container for N motor interfaces with driver management,
/// watchdog, and FreeRTOS thread safety.
/// Pattern identical to RangeArray.
class MotorArray {
 public:
  static constexpr uint32_t WATCHDOG_TIMEOUT_MS = 500;

  MotorArray() = default;

  /// Construct from motor pointers + optional driver groups.
  MotorArray(MotorInterface** motors, uint8_t count,
             const DriverGroupConfig* drivers = nullptr,
             uint8_t num_driver_groups = 0);

  /// Init driver pins + all motors.
  void init();

  /// PID update loop for all motors (call from control task).
  void update();

  // ── driver control ────────────────────────────────────────
  void enableDrivers();
  void disableDrivers();
  bool checkFaults() const;
  /// Latched nFAULT: set after kFaultAssertCycles consecutive control cycles,
  /// cleared kFaultClearMs after the pin releases.
  bool hasFault() const {
    return fault_latched_.load(std::memory_order_relaxed);
  }
  bool isDriversEnabled() const {
    return drivers_enabled_.load(std::memory_order_relaxed);
  }

  // ── bulk operations (mutex-protected) ─────────────────────
  void setVelocities(const float* velocities);
  void stopAll();
  void brakeAll();

  // ── data access ──────────────────────────────────────────
  MotorsData getData() const { return data_; }
  uint8_t count() const { return count_; }
  bool isAvailable() const { return count_ > 0; }

  // ── watchdog ─────────────────────────────────────────────
  void feedWatchdog();
  bool isWatchdogExpired() const;
  void enableWatchdog() {
    watchdog_enabled_.store(true, std::memory_order_relaxed);
  }
  void disableWatchdog() {
    watchdog_enabled_.store(false, std::memory_order_relaxed);
  }

  // ── element access ───────────────────────────────────────
  MotorInterface* operator[](uint8_t idx) {
    return (idx < count_) ? motors_[idx] : nullptr;
  }
  const MotorInterface* operator[](uint8_t idx) const {
    return (idx < count_) ? motors_[idx] : nullptr;
  }

 private:
  MotorInterface** motors_ = nullptr;
  uint8_t count_ = 0;
  const DriverGroupConfig* drivers_ = nullptr;
  uint8_t num_drivers_ = 0;
  MotorsData data_ = {};

  // watchdog
  std::atomic<uint32_t> last_cmd_time_{0};
  std::atomic<bool> watchdog_enabled_{false};
  std::atomic<bool> drivers_enabled_{false};
  uint32_t last_update_us_ = 0;

  // driver fault latch
  static constexpr uint8_t kFaultAssertCycles = 2;
  static constexpr uint32_t kFaultClearMs = 500;
  bool updateFaultLatch(uint32_t now_ms);
  uint8_t fault_cycles_ = 0;
  uint32_t last_fault_ms_ = 0;
  std::atomic<bool> fault_latched_{false};

  // FreeRTOS
  SemaphoreHandle_t mutex_ = nullptr;
};

/// Global motor array — defined in board-specific .cpp
extern MotorArray g_motors;
