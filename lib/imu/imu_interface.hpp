// Copyright 2022 Husarion sp. z o.o.
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

struct ImuData {
  float acceleration[3];      // m/s²
  float angular_velocity[3];  // rad/s
  float orientation[4];       // quaternion x, y, z, w
};

// BNO055 on-chip calibration state, 0-3 per subsystem (3 = fully
// calibrated). Vendor-neutral so persistent_config doesn't need to depend
// on the Adafruit_BNO055 header.
struct ImuCalibrationStatus {
  uint8_t system;
  uint8_t gyro;
  uint8_t accel;
  uint8_t mag;

  bool fullyCalibrated() const {
    return system == 3 && gyro == 3 && accel == 3 && mag == 3;
  }
};

// Mirrors adafruit_bno055_offsets_t (22 bytes, NUM_BNO055_OFFSET_REGISTERS)
// field-for-field so the conversion in ImuBno055 is a straight copy.
struct ImuCalibrationOffsets {
  int16_t accel[3];
  int16_t mag[3];
  int16_t gyro[3];
  int16_t accel_radius;
  int16_t mag_radius;
};

class ImuInterface {
 public:
  virtual ~ImuInterface() = default;

  virtual bool init() = 0;
  virtual void update() = 0;
  virtual const ImuData getData() const { return data_; }
  virtual const char* name() const = 0;

 protected:
  ImuData data_ = {};
};

extern ImuInterface* g_imu;
