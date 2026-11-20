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

#include <cmath>
#include <cstdint>

// DISTANCE_SENSOR carries the range in whole cm and has no "no reading"
// value, so a NaN range (sensor lost or timed out) goes out as
// signal_quality 1, "invalid signal" in the MAVLink spec, with 0 cm. The
// bridge publishes that as NaN; a bridge that ignores signal_quality sees
// 0 cm, which is what NaN cast to uint16_t gave before (-inf in ROS).

namespace range_wire {

inline constexpr uint8_t kSignalQualityUnknown = 0;
inline constexpr uint8_t kSignalQualityInvalid = 1;

struct Reading {
  uint16_t current_cm;
  uint8_t signal_quality;
};

inline Reading encode(float range_m) {
  if (!std::isfinite(range_m) || range_m < 0.0f) {
    return {0, kSignalQualityInvalid};
  }
  const float cm = range_m * 100.0f;
  const uint16_t clamped = cm >= static_cast<float>(UINT16_MAX - 1)
                               ? UINT16_MAX - 1
                               : static_cast<uint16_t>(cm);
  return {clamped, kSignalQualityUnknown};
}

}  // namespace range_wire
