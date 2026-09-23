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

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "rosbot_mavlink_bridge/boot_text.hpp"

using rosbot_mavlink_bridge::BootTextCollector;
using rosbot_mavlink_bridge::isRelayedBootLine;

namespace
{

std::vector<std::string> feedAll(BootTextCollector & c, const std::string & bytes)
{
  std::vector<std::string> lines;
  for (const char ch : bytes) {
    if (auto line = c.feed(static_cast<std::uint8_t>(ch))) {
      lines.push_back(*line);
    }
  }
  return lines;
}

}  // namespace

TEST(BootText, AssemblesCrlfLines)
{
  BootTextCollector c;
  const auto lines = feedAll(
    c,
    "IMU calibration: sys=1 gyro=3 accel=0 mag=2 (12s elapsed)\r\n"
    "IMU calibration: timed out, nothing saved.\r\n");
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0], "IMU calibration: sys=1 gyro=3 accel=0 mag=2 (12s elapsed)");
  EXPECT_EQ(lines[1], "IMU calibration: timed out, nothing saved.");
}

TEST(BootText, BinaryNoiseDropsTheLineInProgress)
{
  BootTextCollector c;
  // A resync fragment ahead of real text on the same line must not leak out
  // as "text", and the line after the newline must still come through.
  const auto lines = feedAll(c, std::string("IMU cal\x01\x9Fibration: x\r\nFW: v2.0.4\r\n"));
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines[0], "FW: v2.0.4");
}

TEST(BootText, OverlongLineIsDropped)
{
  BootTextCollector c;
  const auto lines = feedAll(c, std::string(BootTextCollector::kMaxLine + 10, 'x') + "\nok\n");
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines[0], "ok");
}

TEST(BootText, OnlyCalibrationLinesAreRelayed)
{
  EXPECT_TRUE(isRelayedBootLine("IMU calibration: started, timeout=120s."));
  EXPECT_TRUE(
    isRelayedBootLine(
      "IMU calibration: fully calibrated (sys=3 gyro=3 accel=3 mag=3), offsets captured."));
  EXPECT_TRUE(isRelayedBootLine("IMU init failed: BNO055 not found"));
  EXPECT_FALSE(isRelayedBootLine("FW: v2.0.4-jazzy"));
  EXPECT_FALSE(isRelayedBootLine("  IMU calibration: indented"));
}
