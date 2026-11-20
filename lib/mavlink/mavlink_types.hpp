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

// Sensor samples handed from the producing tasks to the MAVLink publishers.
template <typename T>
struct Stamped {
  T data;
  int64_t timestamp_ns;
};

using BatteryStamped = Stamped<BatteryData>;
using ImuStamped = Stamped<ImuData>;
using JointStateStamped = Stamped<MotorsData>;
