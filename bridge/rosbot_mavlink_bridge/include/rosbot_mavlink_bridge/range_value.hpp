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

#include <cstdint>
#include <limits>

namespace rosbot_mavlink_bridge
{

// DISTANCE_SENSOR -> sensor_msgs/Range value, REP-117: NaN when the reading
// is invalid (signal_quality 1, sent by the firmware for a lost or timed-out
// sensor), -inf below min_range, +inf above max_range.
inline constexpr std::uint8_t kSignalQualityInvalid = 1;

inline float rangeFromDistanceSensor(
  std::uint16_t current_cm, std::uint16_t min_cm, std::uint16_t max_cm,
  std::uint8_t signal_quality)
{
  if (signal_quality == kSignalQualityInvalid) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  if (current_cm > max_cm) {
    return std::numeric_limits<float>::infinity();
  }
  if (current_cm < min_cm) {
    return -std::numeric_limits<float>::infinity();
  }
  return static_cast<float>(current_cm) * 1e-2f;
}

}  // namespace rosbot_mavlink_bridge
