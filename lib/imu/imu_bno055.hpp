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

#include "i2c.hpp"
#include "imu_interface.hpp"
#include "pin.hpp"

namespace bno055 {
// AXIS_MAP_CONFIG / AXIS_MAP_SIGN placements, datasheet section 3.4.
inline constexpr uint8_t kRemapConfigP0 = 0x21;
inline constexpr uint8_t kRemapConfigP1 = 0x24;
inline constexpr uint8_t kRemapConfigP2 = 0x24;
inline constexpr uint8_t kRemapConfigP3 = 0x21;
inline constexpr uint8_t kRemapSignP0 = 0x04;
inline constexpr uint8_t kRemapSignP1 = 0x00;
inline constexpr uint8_t kRemapSignP2 = 0x06;
inline constexpr uint8_t kRemapSignP3 = 0x02;
}  // namespace bno055

struct ImuBno055Config {
  platform::I2c* bus;
  uint8_t i2c_addr;
  platform::Pin int_pin;  // wired, not used yet
  uint8_t axis_config = bno055::kRemapConfigP1;
  uint8_t axis_sign = bno055::kRemapSignP1;
};

class ImuBno055 : public ImuInterface {
 public:
  explicit ImuBno055(const ImuBno055Config& cfg) : cfg_(cfg) {}

  // Resets the chip and starts NDOF fusion with the configured axis remap.
  bool init() override;

  // Task context: reads the data block through DMA.
  bool update() override;
  const char* name() const override { return "BNO055"; }

  // Live CALIB_STAT from the update() block — no extra bus traffic.
  bool calibrationStatus(ImuCalibrationStatus& out) const override;

  // The offset registers are only readable in CONFIG mode, so this leaves
  // NDOF for ~60 ms (no IMU samples, fusion restarts).
  bool readCalibrationOffsets(ImuCalibrationOffsets& out) override;

  // Blocking CALIB_STAT read, for the boot-time calibration window.
  ImuCalibrationStatus getCalibrationStatus();

  // Boot-time: false unless ImuCalibrationStatus::fullyCalibrated().
  bool captureCalibrationOffsets(ImuCalibrationOffsets& out);

  // Loads previously-captured offsets so fusion starts calibrated; handles
  // the CONFIG-mode switch itself.
  void applyCalibrationOffsets(const ImuCalibrationOffsets& offsets);

  // Diagnostic: CALIB_STAT with the I2C outcome. On failure raw_byte is 0
  // and i2c_error holds the HAL_I2C_ERROR_* bits.
  bool probeCalibStatRaw(uint8_t& raw_byte, uint32_t& i2c_error);

 private:
  bool writeByte(uint8_t reg, uint8_t value);
  bool readByte(uint8_t reg, uint8_t& value);
  bool setMode(uint8_t mode);
  bool readOffsetsInConfigMode(ImuCalibrationOffsets& out);

  ImuBno055Config cfg_;
  bool initialized_ = false;
};
