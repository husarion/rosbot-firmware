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

#include <Adafruit_BNO055.h>
#include <Wire.h>

#include "imu_interface.hpp"

struct ImuBno055Config {
  TwoWire* bus;
  uint8_t i2c_addr;
  int32_t sensor_id;
  uint16_t int_pin;
  Adafruit_BNO055::adafruit_bno055_axis_remap_config_t axis_config =
      Adafruit_BNO055::REMAP_CONFIG_P0;
  Adafruit_BNO055::adafruit_bno055_axis_remap_sign_t axis_sign =
      Adafruit_BNO055::REMAP_SIGN_P0;
};

class ImuBno055 : public ImuInterface {
 public:
  explicit ImuBno055(const ImuBno055Config& cfg);

  // Brings the chip up (NDOF mode, axis remap) — safe to follow with
  // blocking Adafruit_BNO055 calls (getCalibrationStatus(),
  // capture/applyCalibrationOffsets()). Does NOT enable the DMA read
  // path used by update() — call enableDmaReads() for that, separately.
  bool init() override;

  // Sets up the DMA + IRQ path update() needs. Must run after init(),
  // and must NOT run before boot-time IMU calibration (if any) is done —
  // it breaks Wire's blocking I2C transactions, which calibration
  // depends on. Call once, right before vTaskStartScheduler(). Until
  // called, update() is a safe no-op (see its s_done_sem guard).
  bool enableDmaReads();

  void update() override;
  const char* name() const override { return "BNO055"; }

  // Not const: Adafruit_BNO055's I2C accessors aren't const-qualified.
  ImuCalibrationStatus getCalibrationStatus();

  // Reads the chip's current offset registers into `out`. Returns false
  // (per Adafruit_BNO055) if the chip isn't fully calibrated yet.
  bool captureCalibrationOffsets(ImuCalibrationOffsets& out);

  // Loads previously-captured offsets into the chip so fusion starts
  // calibrated. Safe to call any time — the driver handles the required
  // CONFIG-mode switch internally.
  void applyCalibrationOffsets(const ImuCalibrationOffsets& offsets);

  // Diagnostic only: reads CALIB_STAT (0x35) directly over `cfg_.bus`,
  // bypassing Adafruit_BNO055::read8() — which silently returns the
  // register address itself (0x35) as "data" on a failed I2C transaction,
  // with no way to tell that apart from a real reading. Returns false
  // (and leaves raw_byte at 0) on I2C failure; `wire_error` is
  // TwoWire::endTransmission()'s code (0 = success, see Arduino Wire
  // docs for 1-5).
  bool probeCalibStatRaw(uint8_t& raw_byte, uint8_t& wire_error);

 private:
  ImuBno055Config cfg_;
  Adafruit_BNO055 bno_;
};
