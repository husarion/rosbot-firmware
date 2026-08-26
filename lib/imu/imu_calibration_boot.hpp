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

#include <HardwareSerial.h>

#include <cstdint>

#include "imu_bno055.hpp"

// Boot-time (pre-scheduler) IMU calibration UI, shared by both variants'
// setup(). Persisting a calibration must happen before
// vTaskStartScheduler() — see persistent_config.hpp — so this never runs
// once ROS/MAVLink are up; the caller is responsible for wiring the
// result into persistent_config::save(). Whether to enter this window at
// all is decided upstream by boot_option::resolveBootAction().
namespace imu_calibration_boot {

// Blocks until the BNO055 reports sys/gyro/accel/mag == 3/3/3/3 or
// `timeout_ms` elapses. `red_led` blinks while waiting; `green_led` (and
// `green_led2`, pass 0 if the board doesn't have a second one) go solid
// on success. Fills `out` and returns true only on success.
//
// `debug_serial` (pass nullptr to disable) gets the sys/gyro/accel/mag
// status printed once a second plus entry/success/timeout lines — this is
// the only feedback available during this window (no ROS/MAVLink link is
// up yet, so `_imu_calibration_status` isn't reachable). The caller must
// pass a line that's actually free — e.g. `CommunicationManager`'s debug
// serial, valid because this window only ever runs on the transport
// branch that leaves it unclaimed (see boot_option.hpp: kCalibrateImu and
// kChangeTransport are mutually exclusive).
bool run(ImuBno055& imu, uint8_t red_led, uint8_t green_led, uint8_t green_led2,
         ImuCalibrationOffsets& out, HardwareSerial* debug_serial = nullptr,
         uint32_t timeout_ms = 120000, uint32_t blink_ms = 150);

}  // namespace imu_calibration_boot
