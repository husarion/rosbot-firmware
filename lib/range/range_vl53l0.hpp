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

#include "VL53L0X.h"
#include "i2c.hpp"
#include "pin.hpp"
#include "range_interface.hpp"

struct RangeVl53l0xConfig {
  platform::I2c* bus;
  platform::Pin xshut_pin;
  uint8_t i2c_address;
  const char* frame_id = "range";
  uint16_t timeout_ms = 500;
  float signal_rate_limit = 0.1f;
  uint8_t vcsel_pre_range = 16;
  uint8_t vcsel_final_range = 12;
  uint32_t timing_budget_us = 50000;
  uint32_t continuous_period_ms = 100;
};

class RangeVl53l0x : public RangeInterface {
 public:
  RangeVl53l0x(const RangeVl53l0xConfig& cfg);

  void init() override;
  void update() override;
  void powerOff() override;
  void powerOn() override;

  bool initialized() const { return initialized_; }
  // Reads that hit the driver timeout since boot.
  uint32_t timeouts() const { return timeouts_; }
  const char* name() const override { return cfg_.frame_id; }
  bool lost() const override { return lost_; }

 private:
  void onTimeout();

  VL53L0X driver_;
  const RangeVl53l0xConfig cfg_;
  bool initialized_ = false;
  bool lost_ = false;
  bool answered_once_ = false;
  uint8_t timeout_streak_ = 0;
  uint32_t not_ready_streak_ = 0;
  uint32_t timeouts_ = 0;
};
