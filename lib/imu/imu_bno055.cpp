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

#include "imu_bno055.hpp"

#include <atomic>

#include "clock.hpp"

namespace {

// BNO055 has acceleration, magnetometer, gyroscope, euler and quaternion
// data laid out contiguously starting at register 0x08, and CALIB_STAT at
// 0x35 a few registers later. One DMA transaction covers the whole range,
// so the live calibration level costs 14 extra bytes, not a second read.
constexpr uint8_t kStartReg = 0x08;                            // ACC_DATA_X_LSB
constexpr uint8_t kCalibStatReg = 0x35;                        // CALIB_STAT
constexpr uint16_t kBlockLen = kCalibStatReg - kStartReg + 1;  // 46 bytes
// 46 B @ 400 kHz is ~1.2 ms on the wire; with the BNO055's clock
// stretching and the task's own wake-up latency the wait measured 1.6-6.9 ms
// (2181 reads, HW 2026-09-23). For ~120 ms after the switch to fusion mode
// the chip stretches longer: an 8 ms limit failed ~12 reads (and reset the
// bus each time) on every XL boot, 25 ms none (HW 2026-11-20). A read past
// the 10 ms period only makes vTaskDelayUntil() run the next one at once.
constexpr uint32_t kReadTimeoutMs = 25;

constexpr uint8_t kChipIdReg = 0x00;
constexpr uint8_t kChipId = 0xA0;
constexpr uint8_t kPageIdReg = 0x07;
constexpr uint8_t kOprModeReg = 0x3D;
constexpr uint8_t kPwrModeReg = 0x3E;
constexpr uint8_t kSysTriggerReg = 0x3F;
constexpr uint8_t kAxisMapConfigReg = 0x41;
constexpr uint8_t kAxisMapSignReg = 0x42;
constexpr uint8_t kOffsetsReg = 0x55;  // ACC_OFFSET_X_LSB .. MAG_RADIUS_MSB
constexpr uint8_t kOffsetsLen = 22;

constexpr uint8_t kModeConfig = 0x00;
constexpr uint8_t kModeNdof = 0x0C;
constexpr uint8_t kPowerNormal = 0x00;
constexpr uint8_t kSysTriggerReset = 0x20;
// Internal oscillator: neither board fits the XIN32/XOUT32 crystal
// (datasheet 5.5); asking for it only costs ~600 ms before the fallback.
constexpr uint8_t kSysTriggerInternalClock = 0x00;

// Settling times as Adafruit_BNO055 1.6.4 used them (datasheet table 3-6:
// any mode -> CONFIG 19 ms, CONFIG -> any 7 ms).
constexpr uint32_t kModeSwitchMs = 30;
constexpr uint32_t kToConfigMs = 25;
constexpr uint32_t kFromConfigMs = 20;
constexpr uint32_t kProbeWindowMs = 850;
constexpr uint32_t kBootRetryMs = 1000;
constexpr uint32_t kResetPollMs = 10;
constexpr uint32_t kResetTimeoutMs = 1000;

constexpr double kDegToRad = 0.017453292519943295769236907684886;

// DMA1 buffers must live in regular SRAM (DMA1 cannot reach CCM on F4).
alignas(4) uint8_t s_buf[kBlockLen];

// CALIB_STAT as of the last good DMA read: sys[7:6] gyro[5:4] accel[3:2]
// mag[1:0]. Written by the IMU task, read by MAVLink. The "never read"
// sentinel sits outside the byte range: 0xFF is a real reading (all four
// channels at 3), and using it as the sentinel silenced the status and
// rejected the save exactly when the chip was fully calibrated.
constexpr uint16_t kCalibStatNeverRead = 0x100;
std::atomic<uint16_t> s_calib_stat{kCalibStatNeverRead};

inline int16_t le16(const uint8_t* p) {
  return static_cast<int16_t>(static_cast<uint16_t>(p[0]) |
                              (static_cast<uint16_t>(p[1]) << 8));
}

// Register order (0x55..0x6A) matches ImuCalibrationOffsets field-for-field.
void unpackOffsets(const uint8_t* raw, ImuCalibrationOffsets& out) {
  int16_t* fields[] = {&out.accel[0],     &out.accel[1],  &out.accel[2],
                       &out.mag[0],       &out.mag[1],    &out.mag[2],
                       &out.gyro[0],      &out.gyro[1],   &out.gyro[2],
                       &out.accel_radius, &out.mag_radius};
  for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
    *fields[i] = le16(&raw[i * 2]);
  }
}

}  // namespace

bool ImuBno055::writeByte(uint8_t reg, uint8_t value) {
  return cfg_.bus->writeReg(cfg_.i2c_addr, reg, &value, 1);
}

bool ImuBno055::readByte(uint8_t reg, uint8_t& value) {
  return cfg_.bus->readReg(cfg_.i2c_addr, reg, &value, 1);
}

bool ImuBno055::setMode(uint8_t mode) {
  const bool ok = writeByte(kOprModeReg, mode);
  platform::clock::delayMs(kModeSwitchMs);
  return ok;
}

// Same order as Adafruit_BNO055::begin() + setAxisRemap/Sign() +
// setExtCrystalUse(false), with the remap and clock source written in the
// one CONFIG-mode window instead of three more mode switches.
bool ImuBno055::init() {
  initialized_ = false;
  uint32_t waited = 0;
  while (!cfg_.bus->probe(cfg_.i2c_addr)) {
    if (waited >= kProbeWindowMs) return false;
    platform::clock::delayMs(kResetPollMs);
    waited += kResetPollMs;
  }

  uint8_t id = 0;
  if (!readByte(kChipIdReg, id) || id != kChipId) {
    platform::clock::delayMs(kBootRetryMs);  // still booting
    if (!readByte(kChipIdReg, id) || id != kChipId) return false;
  }

  setMode(kModeConfig);
  writeByte(kSysTriggerReg, kSysTriggerReset);
  platform::clock::delayMs(30);
  waited = 0;
  while (!readByte(kChipIdReg, id) || id != kChipId) {
    if (waited >= kResetTimeoutMs) return false;
    platform::clock::delayMs(kResetPollMs);
    waited += kResetPollMs;
  }
  platform::clock::delayMs(50);

  bool ok = writeByte(kPwrModeReg, kPowerNormal);
  platform::clock::delayMs(10);
  ok = ok && writeByte(kPageIdReg, 0);
  ok = ok && writeByte(kAxisMapConfigReg, cfg_.axis_config);
  platform::clock::delayMs(10);
  ok = ok && writeByte(kAxisMapSignReg, cfg_.axis_sign);
  platform::clock::delayMs(10);
  ok = ok && writeByte(kSysTriggerReg, kSysTriggerInternalClock);
  platform::clock::delayMs(10);
  ok = ok && setMode(kModeNdof);
  platform::clock::delayMs(kFromConfigMs);

  initialized_ = ok;
  return ok;
}

bool ImuBno055::update() {
  if (!initialized_) return false;
  // A chip that stopped answering costs a bus reset per read; after a
  // streak of failures try once a second instead of every update.
  if (fail_streak_ >= kFailStreakBackoff && ++backoff_skips_ < kBackoffSkips) {
    return false;
  }
  backoff_skips_ = 0;
  const bool ok = cfg_.bus->readRegDma(cfg_.i2c_addr, kStartReg, s_buf,
                                       kBlockLen, kReadTimeoutMs);
  recordRead(ok);
  if (!ok) {
    if (fail_streak_ < kFailStreakBackoff) ++fail_streak_;
    return false;
  }
  fail_streak_ = 0;

  // Acceleration: 1 LSB = 0.01 m/s²  (offsets 0..5)
  data_.acceleration[0] = le16(&s_buf[0]) / 100.0f;
  data_.acceleration[1] = le16(&s_buf[2]) / 100.0f;
  data_.acceleration[2] = le16(&s_buf[4]) / 100.0f;

  // Gyroscope, 1 LSB = 1/16 deg/s: UNIT_SEL stays at its reset value 0x80,
  // as under Adafruit_BNO055 1.6.4 (its UNIT_SEL write is commented out).
  constexpr float kGyroToRad = static_cast<float>(kDegToRad / 16.0);
  data_.angular_velocity[0] = le16(&s_buf[12]) * kGyroToRad;
  data_.angular_velocity[1] = le16(&s_buf[14]) * kGyroToRad;
  data_.angular_velocity[2] = le16(&s_buf[16]) * kGyroToRad;

  // Quaternion: w,x,y,z order in the register block, 1 LSB = 1/2^14
  // (offsets 24..31). ImuData stores x,y,z,w.
  constexpr float kQuatScale = 1.0f / (1 << 14);
  const float qw = le16(&s_buf[24]) * kQuatScale;
  const float qx = le16(&s_buf[26]) * kQuatScale;
  const float qy = le16(&s_buf[28]) * kQuatScale;
  const float qz = le16(&s_buf[30]) * kQuatScale;
  data_.orientation[0] = qx;
  data_.orientation[1] = qy;
  data_.orientation[2] = qz;
  data_.orientation[3] = qw;

  s_calib_stat.store(s_buf[kCalibStatReg - kStartReg]);
  return true;
}

bool ImuBno055::calibrationStatus(ImuCalibrationStatus& out) const {
  const uint16_t raw = s_calib_stat.load();
  if (raw == kCalibStatNeverRead) return false;
  out.system = (raw >> 6) & 0x03;
  out.gyro = (raw >> 4) & 0x03;
  out.accel = (raw >> 2) & 0x03;
  out.mag = raw & 0x03;
  return true;
}

bool ImuBno055::readOffsetsInConfigMode(ImuCalibrationOffsets& out) {
  if (!setMode(kModeConfig)) return false;
  platform::clock::delayMs(kToConfigMs);
  uint8_t raw[kOffsetsLen] = {};
  const bool read_ok =
      cfg_.bus->readReg(cfg_.i2c_addr, kOffsetsReg, raw, kOffsetsLen);
  // Back to fusion whatever the read did — a chip left in CONFIG mode
  // outputs no orientation at all.
  const bool back_ok = setMode(kModeNdof);
  platform::clock::delayMs(kFromConfigMs);
  if (!read_ok || !back_ok) return false;
  unpackOffsets(raw, out);
  return true;
}

bool ImuBno055::readCalibrationOffsets(ImuCalibrationOffsets& out) {
  return initialized_ && readOffsetsInConfigMode(out);
}

ImuCalibrationStatus ImuBno055::getCalibrationStatus() {
  ImuCalibrationStatus status{};
  uint8_t raw = 0;
  if (!readByte(kCalibStatReg, raw)) return status;
  status.system = (raw >> 6) & 0x03;
  status.gyro = (raw >> 4) & 0x03;
  status.accel = (raw >> 2) & 0x03;
  status.mag = raw & 0x03;
  return status;
}

bool ImuBno055::captureCalibrationOffsets(ImuCalibrationOffsets& out) {
  if (!getCalibrationStatus().fullyCalibrated()) return false;
  return readOffsetsInConfigMode(out);
}

void ImuBno055::applyCalibrationOffsets(const ImuCalibrationOffsets& offsets) {
  const int16_t fields[] = {
      offsets.accel[0],     offsets.accel[1],  offsets.accel[2],
      offsets.mag[0],       offsets.mag[1],    offsets.mag[2],
      offsets.gyro[0],      offsets.gyro[1],   offsets.gyro[2],
      offsets.accel_radius, offsets.mag_radius};
  setMode(kModeConfig);
  platform::clock::delayMs(kToConfigMs);
  // One write per register, LSB then MSB, as Adafruit_BNO055 did: the chip
  // latches each pair when its MSB is written.
  uint8_t reg = kOffsetsReg;
  for (const int16_t field : fields) {
    const uint16_t value = static_cast<uint16_t>(field);
    writeByte(reg++, static_cast<uint8_t>(value & 0xFF));
    writeByte(reg++, static_cast<uint8_t>(value >> 8));
  }
  setMode(kModeNdof);
}

bool ImuBno055::probeCalibStatRaw(uint8_t& raw_byte, uint32_t& i2c_error) {
  raw_byte = 0;
  i2c_error = 0;
  if (readByte(kCalibStatReg, raw_byte)) return true;
  raw_byte = 0;
  i2c_error = cfg_.bus->lastError();
  return false;
}
