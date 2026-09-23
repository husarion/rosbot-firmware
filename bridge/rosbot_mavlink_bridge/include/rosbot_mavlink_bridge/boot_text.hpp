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

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace rosbot_mavlink_bridge
{

// Assembles the plain-text lines the firmware prints on the link before
// MAVLink starts (the pre-comm prompt, the boot-time IMU calibration log —
// see lib/imu/imu_calibration_boot.hpp). Fed only the bytes the MAVLink
// parser did not take; anything that is not printable ASCII drops the line
// in progress, so binary noise between frames never surfaces as text.
class BootTextCollector
{
public:
  static constexpr std::size_t kMaxLine = 256;

  std::optional<std::string> feed(std::uint8_t byte)
  {
    if (byte == '\n') {
      std::optional<std::string> out;
      if (!line_.empty() && !dropped_) {
        out = line_;
      }
      line_.clear();
      dropped_ = false;
      return out;
    }
    if (byte == '\r' || dropped_) {
      return std::nullopt;
    }
    if ((byte < 0x20 && byte != '\t') || byte > 0x7E || line_.size() >= kMaxLine) {
      line_.clear();
      dropped_ = true;
      return std::nullopt;
    }
    line_.push_back(static_cast<char>(byte));
    return std::nullopt;
  }

private:
  std::string line_;
  bool dropped_ = false;
};

// Lines worth relaying to the log: the calibration window's progress and
// verdict. The pre-comm prompt ("FW: ...") is left out — it arrives on every
// boot and pre_communication already reports on it.
inline bool isRelayedBootLine(const std::string & line)
{
  return line.rfind("IMU calibration:", 0) == 0 || line.rfind("IMU init failed", 0) == 0;
}

}  // namespace rosbot_mavlink_bridge
