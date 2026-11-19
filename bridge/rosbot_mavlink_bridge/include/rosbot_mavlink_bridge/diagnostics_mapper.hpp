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

#ifndef ROSBOT_MAVLINK_BRIDGE__DIAGNOSTICS_MAPPER_HPP_
#define ROSBOT_MAVLINK_BRIDGE__DIAGNOSTICS_MAPPER_HPP_

#include <cstdint>
#include <string>

#include "diagnostic_msgs/msg/diagnostic_array.hpp"
#include "mavlink.h"  // NOLINT(build/include_subdir) -- mavgen's flat vendored layout

namespace rosbot_mavlink_bridge
{

// Turns ROSBOT_DIAGNOSTICS into a DiagnosticArray. MCU counters only grow,
// so the mapper remembers when each last went up and warns while that was
// recent; an MCU reboot (uptime going back) starts the history over.
class DiagnosticsMapper
{
public:
  explicit DiagnosticsMapper(std::string hardware_id);

  diagnostic_msgs::msg::DiagnosticArray map(
    const mavlink_rosbot_diagnostics_t & d, int64_t now_ns);

  // Thresholds (documented in ROS_API.md).
  static constexpr int64_t kRecentNs = 10'000'000'000;
  static constexpr uint16_t kStackErrorBytes = 64;
  static constexpr uint16_t kStackWarnBytes = 128;
  static constexpr uint32_t kHeapWarnBytes = 4096;
  static constexpr uint16_t kIdleWarnPermille = 200;

private:
  struct Counter
  {
    uint32_t value = 0;
    int64_t last_rise_ns = INT64_MIN;
  };
  // True while the counter went up within kRecentNs.
  bool rose(Counter & c, uint32_t value, int64_t now_ns);

  std::string hardware_id_;
  uint32_t last_uptime_ms_ = 0;
  bool seen_ = false;
  Counter imu_errors_, tx_dropped_, rx_errors_, motor_faults_;
  Counter range_timeouts_[4];
  Counter i2c_recoveries_[2];
};

}  // namespace rosbot_mavlink_bridge

#endif  // ROSBOT_MAVLINK_BRIDGE__DIAGNOSTICS_MAPPER_HPP_
