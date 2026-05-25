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

#include "battery_interface.hpp"
#include "imu_interface.hpp"
#include "motor_array.hpp"
#ifdef ROSBOT
#include "range_array.hpp"
#endif

// Duplicates lib/ros/ros/publishers/*.hpp shapes so the MAVLink build pulls
// no sensor_msgs / micro-ROS headers. Layout must stay binary-compatible.
struct BatteryStamped {
  BatteryData data;
  int64_t timestamp_ns;
};

struct ImuStamped {
  ImuData data;
  int64_t timestamp_ns;
};

struct JointStateStamped {
  MotorsData data;
  int64_t timestamp_ns;
};

#ifdef ROSBOT
struct RangesStamped {
  RangesData data;
  int64_t timestamp_ns;
};
#endif
