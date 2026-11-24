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

// BATTERY_STATUS has no health or Celsius field. Temperature goes in
// `temperature` (cdegC, INT16_MAX = unknown); health goes in `charge_state`
// plus `fault_bitmask`, as the MAVLink spec requires for faults. Values below
// are the MAV_BATTERY_CHARGE_STATE_* / MAV_BATTERY_FAULT_* numbers
// (battery_publisher.hpp static_asserts them); the bridge decodes the same
// table. Old firmware sends charge_state 0 and old bridges ignore both fields.

namespace battery_wire {

inline constexpr int16_t kTemperatureUnknown = INT16_MAX;

inline constexpr uint8_t kChargeStateUndefined = 0;
inline constexpr uint8_t kChargeStateOk = 1;
inline constexpr uint8_t kChargeStateFailed = 5;
inline constexpr uint8_t kChargeStateUnhealthy = 6;

inline constexpr uint32_t kFaultCellFail = 4;
inline constexpr uint32_t kFaultOverTemperature = 16;
inline constexpr uint32_t kFaultUnderTemperature = 32;

struct Health {
  uint8_t charge_state;
  uint32_t fault_bitmask;
};

// BATTERY_STATUS.current_battery uses -1 for "not measured"; -0.01 A is
// sent as -0.02 A so a real reading never collides with it.
inline constexpr int16_t kCurrentUnknown = -1;

inline int16_t currentToCentiAmps(float amps) {
  if (!std::isfinite(amps)) return kCurrentUnknown;
  const long ca = std::lround(amps * 100.0f);
  if (ca > INT16_MAX) return INT16_MAX;
  if (ca < INT16_MIN + 1) return INT16_MIN + 1;
  return ca == kCurrentUnknown ? kCurrentUnknown - 1 : static_cast<int16_t>(ca);
}

// ROSBOT_POWER_SUPPLY.capacity_mah: 0 = unknown.
inline uint16_t capacityToMah(float amp_hours) {
  if (!std::isfinite(amp_hours) || amp_hours <= 0.0f) return 0;
  const long mah = std::lround(amp_hours * 1000.0f);
  return mah > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(mah);
}

inline int16_t temperatureToCentiDegrees(float celsius) {
  if (!std::isfinite(celsius) || celsius < -300.0f || celsius > 300.0f) {
    return kTemperatureUnknown;
  }
  return static_cast<int16_t>(std::lround(celsius * 100.0f));
}

// `health` uses sensor_msgs/BatteryState POWER_SUPPLY_HEALTH_* numbering;
// anything unknown stays "not provided". WATCHDOG/SAFETY_TIMER_EXPIRE (7, 8)
// have no MAVLink fault and arrive as a plain failure.
inline Health healthToWire(uint8_t health) {
  switch (health) {
    case 1:
      return {kChargeStateOk, 0};
    case 2:
      return {kChargeStateUnhealthy, kFaultOverTemperature};
    case 3:
      return {kChargeStateFailed, kFaultCellFail};
    case 6:
      return {kChargeStateUnhealthy, kFaultUnderTemperature};
    case 4:
    case 5:
    case 7:
    case 8:
      return {kChargeStateUnhealthy, 0};
    default:
      return {kChargeStateUndefined, 0};
  }
}

}  // namespace battery_wire
