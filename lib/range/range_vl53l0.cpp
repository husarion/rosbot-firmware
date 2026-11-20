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

#include "range_vl53l0.hpp"

#include <cmath>

#include "clock.hpp"

static constexpr uint8_t RESULT_READY_MASK = 0x07;
static constexpr uint8_t kLostAfterTimeouts = 2;

RangeVl53l0x::RangeVl53l0x(const RangeVl53l0xConfig& cfg) : cfg_(cfg) {}

void RangeVl53l0x::init() {
  lost_ = false;
  timeout_streak_ = 0;
  not_ready_streak_ = 0;
  powerOn();

  // XSHUT put the sensor back at the default address; the driver object
  // still holds the one setAddress() gave it on the previous init().
  driver_ = VL53L0X();
  driver_.setBus(cfg_.bus);
  driver_.setTimeout(cfg_.timeout_ms);
  if (!driver_.init()) return;

  driver_.setAddress(cfg_.i2c_address);
  driver_.setSignalRateLimit(cfg_.signal_rate_limit);
  driver_.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange,
                              cfg_.vcsel_pre_range);
  driver_.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange,
                              cfg_.vcsel_final_range);
  driver_.setMeasurementTimingBudget(cfg_.timing_budget_us);
  driver_.startContinuous(cfg_.continuous_period_ms);

  initialized_ = true;
}

void RangeVl53l0x::update() {
  if (!initialized_) {
    data_.range = NAN;
    return;
  }

  // One status read instead of the driver's wait: it polls the register
  // until data is ready, which is up to a whole measurement period of I2C
  // traffic when the sensors' phase drifts from this task's (seen as ~25 %
  // CPU after a re-init on HW) and the full timeout for a dead sensor.
  const uint8_t status = driver_.readReg(VL53L0X::RESULT_INTERRUPT_STATUS);
  if (driver_.last_status != 0) {
    onTimeout();
    return;
  }
  if ((status & RESULT_READY_MASK) == 0) {
    // Keep the last reading; give up after about timeout_ms without data.
    if (++not_ready_streak_ * cfg_.continuous_period_ms >= cfg_.timeout_ms) {
      not_ready_streak_ = 0;
      onTimeout();
    }
    return;
  }
  not_ready_streak_ = 0;
  const uint16_t range_mm = driver_.readRangeContinuousMillimeters();
  if (driver_.timeoutOccurred()) {
    onTimeout();
    return;
  }
  timeout_streak_ = 0;
  data_.range = static_cast<float>(range_mm) / 1000.0f;
}

void RangeVl53l0x::onTimeout() {
  ++timeouts_;
  data_.range = NAN;
  if (++timeout_streak_ >= kLostAfterTimeouts) {
    lost_ = true;
    initialized_ = false;
  }
}

void RangeVl53l0x::powerOff() {
  cfg_.xshut_pin.setAsOutput(false);
  initialized_ = false;
  platform::clock::delayMs(10);
}

void RangeVl53l0x::powerOn() {
  cfg_.xshut_pin.setAsOutput(true);
  platform::clock::delayMs(20);
}
