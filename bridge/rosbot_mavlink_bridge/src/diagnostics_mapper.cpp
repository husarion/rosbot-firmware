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

#include "rosbot_mavlink_bridge/diagnostics_mapper.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>

using diagnostic_msgs::msg::DiagnosticStatus;
using diagnostic_msgs::msg::KeyValue;

namespace rosbot_mavlink_bridge
{
namespace
{

// ROSBOT_DIAGNOSTICS.flags and reset_cause bits (rosbot.xml).
constexpr uint8_t kBatteryLow = 1;
constexpr uint8_t kBatteryCritical = 2;
constexpr uint8_t kMotorFault = 4;
constexpr uint8_t kPowerBoardOk = 8;
constexpr uint8_t kEthernetUp = 16;
constexpr uint8_t kHasRanges = 32;
constexpr uint8_t kHasPowerBoard = 64;
constexpr uint8_t kHasEthernet = 128;
constexpr uint16_t kResetFirmwareFault = 64;

const char * const kRangeNames[4] = {"front left", "front right", "rear left", "rear right"};

std::string resetCauseText(uint16_t cause)
{
  static const std::pair<uint16_t, const char *> kNames[] = {
    {1, "reset pin"}, {2, "power-on/brown-out"}, {4, "software"},
    {8, "independent watchdog"}, {16, "window watchdog"}, {32, "low-power"},
    {64, "firmware fault"}};
  std::string out;
  for (const auto & [bit, name] : kNames) {
    if (cause & bit) {
      out += out.empty() ? "" : ", ";
      out += name;
    }
  }
  return out.empty() ? "unknown" : out;
}

// ROSBOT_DIAGNOSTICS.fault_code, with the PC or task when there is one.
std::string faultText(const mavlink_rosbot_diagnostics_t & d)
{
  static const char * const kNames[] = {
    "unknown", "assert", "stack overflow", "out of heap", "hard fault",
    "memory-management fault", "bus fault", "usage fault", "NMI",
    "clock start failure", "libc exit"};
  std::string out = d.fault_code < sizeof(kNames) / sizeof(kNames[0]) ?
    kNames[d.fault_code] : "code " + std::to_string(d.fault_code);
  if (d.fault_code == 2) {
    out += " in " + std::string(d.fault_task, strnlen(d.fault_task, sizeof(d.fault_task)));
  } else if (d.fault_pc != 0) {
    char pc[16];
    snprintf(pc, sizeof(pc), " at 0x%08x", static_cast<unsigned>(d.fault_pc));
    out += pc;
  }
  return out;
}

class StatusBuilder
{
public:
  StatusBuilder(const std::string & name, const std::string & hardware_id)
  {
    s_.name = hardware_id + ": " + name;
    s_.hardware_id = hardware_id;
    s_.level = DiagnosticStatus::OK;
  }
  // Keeps the worst level; the message of the worst one wins.
  void raise(uint8_t level, const std::string & message)
  {
    if (level > s_.level || s_.message.empty()) {
      s_.level = level > s_.level ? level : s_.level;
      s_.message = message;
    }
  }
  template<typename T>
  void add(const std::string & key, const T & value)
  {
    KeyValue kv;
    kv.key = key;
    if constexpr (std::is_same_v<T, std::string>) {
      kv.value = value;
    } else {
      kv.value = std::to_string(value);
    }
    s_.values.push_back(kv);
  }
  DiagnosticStatus done()
  {
    if (s_.message.empty()) {s_.message = "OK";}
    return s_;
  }

private:
  DiagnosticStatus s_;
};

}  // namespace

DiagnosticsMapper::DiagnosticsMapper(std::string hardware_id)
: hardware_id_(std::move(hardware_id)) {}

bool DiagnosticsMapper::rose(Counter & c, uint32_t value, int64_t now_ns)
{
  if (value > c.value) {c.last_rise_ns = now_ns;}
  c.value = value;
  return c.last_rise_ns != INT64_MIN && now_ns - c.last_rise_ns < kRecentNs;
}

diagnostic_msgs::msg::DiagnosticArray DiagnosticsMapper::map(
  const mavlink_rosbot_diagnostics_t & d, int64_t now_ns)
{
  if (seen_ && d.uptime_ms < last_uptime_ms_) {
    *this = DiagnosticsMapper(hardware_id_);  // MCU rebooted
  }
  // The first message only primes the counters: what accumulated before the
  // bridge started is not "recent".
  const bool first = !seen_;
  seen_ = true;
  last_uptime_ms_ = d.uptime_ms;
  auto grew = [&](Counter & c, uint32_t v) {
      if (first) {
        c.value = v;
        return false;
      }
      return rose(c, v, now_ns);
    };

  diagnostic_msgs::msg::DiagnosticArray out;

  StatusBuilder mcu("MCU", hardware_id_);
  const std::string stack_task(d.stack_min_task,
    strnlen(d.stack_min_task, sizeof(d.stack_min_task)));
  if (d.stack_min_free < kStackErrorBytes) {
    mcu.raise(DiagnosticStatus::ERROR, "Stack nearly exhausted in " + stack_task);
  } else if (d.stack_min_free < kStackWarnBytes) {
    mcu.raise(DiagnosticStatus::WARN, "Low stack headroom in " + stack_task);
  }
  if (d.heap_min_free < kHeapWarnBytes) {
    mcu.raise(DiagnosticStatus::WARN, "Heap nearly exhausted");
  }
  const bool idle_known = d.cpu_idle != UINT16_MAX;
  if (idle_known && d.cpu_idle < kIdleWarnPermille) {
    mcu.raise(DiagnosticStatus::WARN, "CPU overloaded");
  }
  if (d.reset_cause & kResetFirmwareFault) {
    mcu.raise(DiagnosticStatus::WARN, "Last reset was a firmware fault: " + faultText(d));
    mcu.add("Last fault", faultText(d));
  }
  mcu.add("Uptime [s]", d.uptime_ms / 1000);
  mcu.add("Last reset", resetCauseText(d.reset_cause));
  if (idle_known) {
    mcu.add("CPU idle [%]", d.cpu_idle / 10.0);
  } else {
    mcu.add("CPU idle [%]", std::string("n/a"));
  }
  mcu.add("Heap free [B]", d.heap_free);
  mcu.add("Heap never used [B]", d.heap_min_free);
  mcu.add("Smallest stack headroom [B]", d.stack_min_free);
  mcu.add("Task with smallest stack", stack_task);
  out.status.push_back(mcu.done());

  StatusBuilder imu("IMU", hardware_id_);
  const bool imu_errors = grew(imu_errors_, d.imu_errors);
  const bool imu_bus_reset = grew(i2c_recoveries_[0], d.i2c_recoveries[0]);
  if (!d.imu_ok) {
    imu.raise(DiagnosticStatus::ERROR, "IMU not responding");
  } else if (imu_errors) {
    imu.raise(DiagnosticStatus::WARN, "IMU read errors");
  } else if (imu_bus_reset) {
    imu.raise(DiagnosticStatus::WARN, "IMU bus was reset");
  }
  imu.add("Read errors", d.imu_errors);
  imu.add("Bus resets", d.i2c_recoveries[0]);
  out.status.push_back(imu.done());

  if (d.flags & kHasRanges) {
    StatusBuilder ranges("Range sensors", hardware_id_);
    for (size_t i = 0; i < 4; ++i) {
      const bool timing_out = grew(range_timeouts_[i], d.range_timeouts[i]);
      if (!(d.range_ok & (1U << i))) {
        ranges.raise(DiagnosticStatus::ERROR, std::string(kRangeNames[i]) + " sensor not found");
      } else if (timing_out) {
        ranges.raise(DiagnosticStatus::WARN, std::string(kRangeNames[i]) + " sensor timing out");
      }
      ranges.add(std::string(kRangeNames[i]) + " timeouts", d.range_timeouts[i]);
    }
    if (grew(i2c_recoveries_[1], d.i2c_recoveries[1])) {
      ranges.raise(DiagnosticStatus::WARN, "Sensor bus was reset");
    }
    ranges.add("Bus resets", d.i2c_recoveries[1]);
    out.status.push_back(ranges.done());
  }

  StatusBuilder motors("Motors", hardware_id_);
  const bool faults_rising = grew(motor_faults_, d.motor_faults);
  if (d.flags & kMotorFault) {
    motors.raise(DiagnosticStatus::ERROR, "Motor driver fault active");
  } else if (faults_rising) {
    motors.raise(DiagnosticStatus::WARN, "Motor driver faults");
  }
  motors.add("Driver fault events", d.motor_faults);
  out.status.push_back(motors.done());

  StatusBuilder link("Link", hardware_id_);
  const bool tx = grew(tx_dropped_, d.link_tx_dropped);
  const bool rx = grew(rx_errors_, d.link_rx_errors);
  if (tx) {link.raise(DiagnosticStatus::WARN, "MCU dropping telemetry (TX queue full)");}
  if (rx) {link.raise(DiagnosticStatus::WARN, "MCU receiving corrupted frames");}
  link.add("TX frames dropped", d.link_tx_dropped);
  link.add("RX frames dropped", d.link_rx_errors);
  link.add("Connections", d.link_reconnects);
  out.status.push_back(link.done());

  StatusBuilder power("Power", hardware_id_);
  if (d.flags & kBatteryCritical) {
    power.raise(DiagnosticStatus::ERROR, "Battery critical");
  } else if (d.flags & kBatteryLow) {
    power.raise(DiagnosticStatus::WARN, "Battery low");
  }
  if ((d.flags & kHasPowerBoard) && !(d.flags & kPowerBoardOk)) {
    power.raise(DiagnosticStatus::ERROR, "Power board not answering");
  }
  if (d.flags & kHasPowerBoard) {
    power.add("Power board answering", std::string(d.flags & kPowerBoardOk ? "yes" : "no"));
  }
  if (d.flags & kHasEthernet) {
    power.add("Ethernet link", std::string(d.flags & kEthernetUp ? "up" : "down"));
  }
  out.status.push_back(power.done());

  return out;
}

}  // namespace rosbot_mavlink_bridge
