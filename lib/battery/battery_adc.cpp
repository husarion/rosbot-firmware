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

#include <cmath>

#include "time_base.hpp"

BatteryAdc::BatteryAdc(const BatteryAdcConfig config)
    // ROSbot 3 cells (3500 mAh 18650) were never measured; the XL pack's
    // curve is much closer than the generic one (6 % vs ~42 % at 10.9 V).
    : cfg_(config),
      soc_(SocEstimatorConfig{.num_cells = config.num_cells,
                              .ocv = &ocv_curves::kRosbotXl}) {
  voltage_factor_ = cfg_.v_ref * cfg_.correction * cfg_.divider;
}

void BatteryAdc::init() { cfg_.adc.init(); }

void BatteryAdc::update() {
  float raw = cfg_.adc.read() * cfg_.adc_resolution_scale;

  data_.voltage = raw * voltage_factor_;

  const uint32_t now_ms = time_boot_ms();
  const float dt_s =
      (last_update_ms_ == 0)
          ? 0.0f
          : static_cast<float>(now_ms - last_update_ms_) * 0.001f;
  last_update_ms_ = now_ms;
  data_.percentage = soc_.updateVoltageOnly(data_.voltage, dt_s);
}
