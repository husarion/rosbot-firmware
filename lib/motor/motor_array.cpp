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

#include "motor_array.hpp"

#include <Arduino.h>
#include <FreeRTOS.h>
#include <etl/algorithm.h>

MotorArray::MotorArray(etl::span<MotorInterface* const> motors,
                       etl::span<const DriverGroupConfig> drivers)
    : motors_(motors.first(etl::min(motors.size(), size_t{MAX_NUM_MOTORS}))),
      drivers_(drivers) {}

// ─── init ─────────────────────────────────────────────────────────

void MotorArray::init() {
  mutex_ = xSemaphoreCreateMutex();
  configASSERT(mutex_ != nullptr);

  // Setup driver control pins
  for (const auto& driver : drivers_) {
    driver.sleep_pin.setAsOutput(false);
    driver.fault_pin.setAsInput(platform::Pull::kUp);
  }
  disableDrivers();

  // Init all motors
  data_.count = count();
  for (auto* motor : motors_) motor->init();

  last_update_us_ = micros();
  enableDrivers();
}

// ─── driver control ───────────────────────────────────────────────

void MotorArray::enableDrivers() {
  for (const auto& driver : drivers_) driver.sleep_pin.set();
  drivers_enabled_.store(true, std::memory_order_relaxed);
  delayMicroseconds(100);  // DRV8848 wake-up
}

void MotorArray::disableDrivers() {
  for (const auto& driver : drivers_) driver.sleep_pin.reset();
  drivers_enabled_.store(false, std::memory_order_relaxed);
}

bool MotorArray::checkFaults() const {
  for (const auto& driver : drivers_)
    if (!driver.fault_pin.read()) return true;
  return false;
}

// ─── bulk operations (mutex-protected) ────────────────────────────

void MotorArray::setVelocities(const float* velocities) {
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
    for (size_t i = 0; i < motors_.size(); ++i)
      motors_[i]->setVelocity(velocities[i]);
    feedWatchdog();
    xSemaphoreGive(mutex_);
  }
}

// ─── update (PID loop — called from control task) ────────────────

void MotorArray::update() {
  if (!isDriversEnabled()) return;

  const uint32_t now_us = micros();
  const float dt = (now_us - last_update_us_) * 1e-6f;
  last_update_us_ = now_us;

  const bool fault = updateFaultLatch(millis());

  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(5)) == pdTRUE) {
    for (size_t i = 0; i < motors_.size(); ++i) {
      // update(dt, false) brakes, which zeroes the target and the PID every
      // cycle: setpoints that arrive during a fault are dropped, and the
      // wheels move again only on a fresh one, without a stale integral.
      motors_[i]->update(dt, !fault && !isWatchdogExpired());
      const auto d = motors_[i]->getData();
      data_.position[i] = d.position;
      data_.velocity[i] = d.velocity;
      data_.effort[i] = d.effort;
    }
    xSemaphoreGive(mutex_);
  }
}

bool MotorArray::updateFaultLatch(uint32_t now_ms) {
  bool latched = fault_latched_.load(std::memory_order_relaxed);
  if (checkFaults()) {
    last_fault_ms_ = now_ms;
    if (fault_cycles_ < kFaultAssertCycles) ++fault_cycles_;
    if (fault_cycles_ >= kFaultAssertCycles) latched = true;
  } else {
    fault_cycles_ = 0;
    if (latched && now_ms - last_fault_ms_ >= kFaultClearMs) latched = false;
  }
  fault_latched_.store(latched, std::memory_order_relaxed);
  return latched;
}

// ─── watchdog ────────────────────────────────────────────────────

void MotorArray::feedWatchdog() {
  last_cmd_time_.store(millis(), std::memory_order_relaxed);
  watchdog_enabled_.store(true, std::memory_order_relaxed);
}

bool MotorArray::isWatchdogExpired() const {
  if (!watchdog_enabled_.load(std::memory_order_relaxed)) return false;
  return (millis() - last_cmd_time_.load(std::memory_order_relaxed)) >
         WATCHDOG_TIMEOUT_MS;
}
