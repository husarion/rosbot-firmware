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

#include <cstring>
#include <string>

#include "rosbot_mavlink_bridge/diagnostics_mapper.hpp"

using diagnostic_msgs::msg::DiagnosticArray;
using diagnostic_msgs::msg::DiagnosticStatus;
using rosbot_mavlink_bridge::DiagnosticsMapper;

namespace
{

constexpr int64_t kSec = 1'000'000'000;

mavlink_rosbot_diagnostics_t healthyRosbot(uint32_t uptime_ms)
{
  mavlink_rosbot_diagnostics_t d{};
  d.uptime_ms = uptime_ms;
  d.cpu_idle = 960;
  d.heap_free = 99000;
  d.heap_min_free = 98000;
  d.stack_min_free = 340;
  std::strncpy(d.stack_min_task, "Range", sizeof(d.stack_min_task));
  d.reset_cause = 2;
  d.imu_ok = 1;
  d.range_ok = 0x0F;
  d.flags = 32;  // has range sensors
  return d;
}

const DiagnosticStatus & status(const DiagnosticArray & a, const std::string & name)
{
  for (const auto & s : a.status) {
    if (s.name == "rosbot_mcu: " + name) {return s;}
  }
  static DiagnosticStatus missing;
  missing.name = "MISSING " + name;
  missing.level = 255;
  return missing;
}

std::string value(const DiagnosticStatus & s, const std::string & key)
{
  for (const auto & kv : s.values) {
    if (kv.key == key) {return kv.value;}
  }
  return "<none>";
}

}  // namespace

TEST(DiagnosticsMapper, HealthyRobotIsAllOk)
{
  DiagnosticsMapper m("rosbot_mcu");
  const auto a = m.map(healthyRosbot(5000), 0);
  ASSERT_EQ(a.status.size(), 6u);  // MCU, IMU, ranges, motors, link, power
  for (const auto & s : a.status) {
    EXPECT_EQ(s.level, DiagnosticStatus::OK) << s.name << ": " << s.message;
    EXPECT_EQ(s.hardware_id, "rosbot_mcu");
  }
  EXPECT_EQ(value(status(a, "MCU"), "Last reset"), "power-on/brown-out");
  EXPECT_EQ(value(status(a, "MCU"), "Task with smallest stack"), "Range");
}

TEST(DiagnosticsMapper, MissingImuAndSensorAreErrors)
{
  DiagnosticsMapper m("x");
  auto d = healthyRosbot(5000);
  d.imu_ok = 0;
  d.range_ok = 0x0D;  // front right missing
  const auto a = m.map(d, 0);
  EXPECT_EQ(status(a, "IMU").level, DiagnosticStatus::ERROR);
  EXPECT_EQ(status(a, "Range sensors").level, DiagnosticStatus::ERROR);
  EXPECT_NE(status(a, "Range sensors").message.find("front right"), std::string::npos);
}

TEST(DiagnosticsMapper, OldCountersAreNotRecentButNewOnesWarnForAWhile)
{
  DiagnosticsMapper m("x");
  auto d = healthyRosbot(5000);
  d.range_timeouts[1] = 40;  // happened before the bridge started
  EXPECT_EQ(status(m.map(d, 0), "Range sensors").level, DiagnosticStatus::OK);

  d.uptime_ms = 6000;
  d.range_timeouts[1] = 41;
  EXPECT_EQ(status(m.map(d, 1 * kSec), "Range sensors").level, DiagnosticStatus::WARN);

  d.uptime_ms = 10000;  // no change, still within the window
  EXPECT_EQ(status(m.map(d, 5 * kSec), "Range sensors").level, DiagnosticStatus::WARN);

  d.uptime_ms = 20000;
  EXPECT_EQ(status(m.map(d, 12 * kSec), "Range sensors").level, DiagnosticStatus::OK);
}

TEST(DiagnosticsMapper, McuRebootRestartsHistory)
{
  DiagnosticsMapper m("x");
  auto d = healthyRosbot(50000);
  m.map(d, 0);
  d.link_tx_dropped = 7;
  d.uptime_ms = 51000;
  EXPECT_EQ(status(m.map(d, kSec), "Link").level, DiagnosticStatus::WARN);

  auto fresh = healthyRosbot(1000);  // uptime went back: new boot
  fresh.link_tx_dropped = 3;
  EXPECT_EQ(status(m.map(fresh, 2 * kSec), "Link").level, DiagnosticStatus::OK);
}

TEST(DiagnosticsMapper, McuResourceThresholds)
{
  DiagnosticsMapper m("x");
  auto d = healthyRosbot(5000);
  d.stack_min_free = DiagnosticsMapper::kStackWarnBytes - 1;
  EXPECT_EQ(status(m.map(d, 0), "MCU").level, DiagnosticStatus::WARN);
  d.stack_min_free = DiagnosticsMapper::kStackErrorBytes - 1;
  EXPECT_EQ(status(m.map(d, 0), "MCU").level, DiagnosticStatus::ERROR);

  auto e = healthyRosbot(5000);
  e.cpu_idle = DiagnosticsMapper::kIdleWarnPermille - 1;
  EXPECT_EQ(status(m.map(e, 0), "MCU").level, DiagnosticStatus::WARN);

  auto u = healthyRosbot(5000);
  u.cpu_idle = UINT16_MAX;  // first message after boot
  EXPECT_EQ(status(m.map(u, 0), "MCU").level, DiagnosticStatus::OK);
  EXPECT_EQ(value(status(m.map(u, 0), "MCU"), "CPU idle [%]"), "n/a");

  auto f = healthyRosbot(5000);
  f.reset_cause = 4 | 64;  // software + firmware fault
  const auto a = m.map(f, 0);
  EXPECT_EQ(status(a, "MCU").level, DiagnosticStatus::WARN);
  EXPECT_EQ(value(status(a, "MCU"), "Last reset"), "software, firmware fault");
  EXPECT_EQ(value(status(a, "MCU"), "Last fault"), "unknown");
}

TEST(DiagnosticsMapper, FaultDetailNamesThePcOrTheTask)
{
  DiagnosticsMapper m("x");
  auto d = healthyRosbot(5000);
  d.reset_cause = 4 | 64;
  d.fault_code = 4;
  d.fault_pc = 0x08001234;
  EXPECT_EQ(value(status(m.map(d, 0), "MCU"), "Last fault"), "hard fault at 0x08001234");
  d.fault_code = 2;
  std::strncpy(d.fault_task, "Imu", sizeof(d.fault_task));
  const auto a = m.map(d, 0);
  EXPECT_EQ(value(status(a, "MCU"), "Last fault"), "stack overflow in Imu");
  EXPECT_NE(status(a, "MCU").message.find("stack overflow in Imu"), std::string::npos);
}

TEST(DiagnosticsMapper, MotorAndBatteryFlags)
{
  DiagnosticsMapper m("x");
  auto d = healthyRosbot(5000);
  d.flags |= 4 | 1;  // motor fault active, battery low
  auto a = m.map(d, 0);
  EXPECT_EQ(status(a, "Motors").level, DiagnosticStatus::ERROR);
  EXPECT_EQ(status(a, "Power").level, DiagnosticStatus::WARN);
  d.flags |= 2;  // critical
  EXPECT_EQ(status(m.map(d, 0), "Power").level, DiagnosticStatus::ERROR);
}

TEST(DiagnosticsMapper, RosbotXlReportsPowerBoardAndEthernet)
{
  DiagnosticsMapper m("x");
  mavlink_rosbot_diagnostics_t d{};
  d.uptime_ms = 1000;
  d.cpu_idle = 900;
  d.heap_min_free = 40000;
  d.stack_min_free = 300;
  d.imu_ok = 1;
  d.flags = 64 | 128 | 16;  // power board + Ethernet present, link up, PB silent
  const auto a = m.map(d, 0);
  EXPECT_EQ(status(a, "Range sensors").level, 255);  // not reported on XL
  EXPECT_EQ(status(a, "Power").level, DiagnosticStatus::ERROR);
  EXPECT_EQ(value(status(a, "Power"), "Ethernet link"), "up");
}
