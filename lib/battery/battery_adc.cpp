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

#include <cmath>

BatteryAdc::BatteryAdc(const BatteryAdcConfig config)
    : cfg_(config), soc_(SocEstimatorConfig{.num_cells = config.num_cells}) {
  voltage_factor_ = cfg_.v_ref * cfg_.correction * cfg_.divider;
}

void BatteryAdc::init() { pinMode(cfg_.adc_pin, INPUT); }

void BatteryAdc::update() {
  float raw = analogRead(cfg_.adc_pin) * cfg_.adc_resolution_scale;

  data_.voltage = raw * voltage_factor_;

  const uint32_t now_ms = millis();
  const float dt_s =
      (last_update_ms_ == 0)
          ? 0.0f
          : static_cast<float>(now_ms - last_update_ms_) * 0.001f;
  last_update_ms_ = now_ms;
  data_.percentage = soc_.updateVoltageOnly(data_.voltage, dt_s);
}
