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

#include "battery_interface.hpp"
#include "mavlink_node.hpp"
#include "motor_array.hpp"

// Run from the link task only: isCritical() keeps hysteresis state, and
// STATUSTEXT formatting needs that task's stack.
class CriticalStateReporter {
 public:
  void update() {
    const bool motor_fault = g_motors.hasFault();
    const bool battery_critical = g_battery->isCritical();
    g_mavlink_node.setCritical(motor_fault || battery_critical);
    report(motor_fault_, motor_fault, "Motor driver fault");
    report(battery_critical_, battery_critical, "Battery critical");
  }

 private:
  static void report(bool& reported, bool active, const char* what) {
    if (active == reported) return;
    reported = active;
    g_mavlink_node.log(active ? MAV_SEVERITY_CRITICAL : MAV_SEVERITY_NOTICE,
                       active ? "%s" : "%s cleared", what);
  }

  bool motor_fault_ = false;
  bool battery_critical_ = false;
};
