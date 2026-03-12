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

#include "battery_adc.hpp"

#include <Arduino.h>

#include <cassert>
#include <cmath>

BatteryAdc::BatteryAdc(const BatteryAdcConfig config) : cfg_(config) {
  voltage_factor_ = cfg_.v_ref * cfg_.correction * cfg_.divider;
  assert(cfg_.v_max > cfg_.v_min);
  voltage_range_inv_ = 1.0f / (cfg_.v_max - cfg_.v_min);
}

void BatteryAdc::init() { pinMode(cfg_.adc_pin, INPUT); }

void BatteryAdc::update() {
  float raw = analogRead(cfg_.adc_pin) * cfg_.adc_resolution_scale;

  data_.voltage = raw * voltage_factor_;

  float pct = (data_.voltage - cfg_.v_min) * voltage_range_inv_;
  data_.percentage = fminf(fmaxf(pct, 0.0f), 1.0f);
}
