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

#include "imu_calibration_boot.hpp"

#include <Arduino.h>

namespace imu_calibration_boot {

namespace {
constexpr uint32_t kLogIntervalMs = 1000;
}  // namespace

bool run(ImuBno055& imu, uint8_t red_led, uint8_t green_led, uint8_t green_led2,
         ImuCalibrationOffsets& out, HardwareSerial* debug_serial,
         uint32_t timeout_ms, uint32_t blink_ms) {
  const uint32_t start = millis();
  uint32_t last_toggle = start;
  uint32_t last_log = start;
  bool led_on = false;

  if (debug_serial != nullptr) {
    debug_serial->printf(
        "IMU calibration: started, timeout=%lus. Move the robot — "
        "sit still (gyro), figure-8 in the air (mag), hold a few tilted "
        "rests >45deg apart (accel).\r\n",
        static_cast<unsigned long>(timeout_ms / 1000));
  }

  while (millis() - start < timeout_ms) {
    const ImuCalibrationStatus status = imu.getCalibrationStatus();
    if (status.fullyCalibrated()) {
      digitalWrite(red_led, LOW);
      digitalWrite(green_led, HIGH);
      if (green_led2 != 0) digitalWrite(green_led2, HIGH);
      const bool captured = imu.captureCalibrationOffsets(out);
      if (debug_serial != nullptr) {
        debug_serial->printf(
            "IMU calibration: fully calibrated (sys=3 gyro=3 accel=3 "
            "mag=3), offsets %s.\r\n",
            captured ? "captured" : "capture FAILED");
      }
      return captured;
    }
    const uint32_t now = millis();
    if (now - last_toggle >= blink_ms) {
      led_on = !led_on;
      digitalWrite(red_led, led_on ? HIGH : LOW);
      last_toggle = now;
    }
    if (debug_serial != nullptr && now - last_log >= kLogIntervalMs) {
      uint8_t raw_byte = 0;
      uint8_t wire_error = 0;
      const bool i2c_ok = imu.probeCalibStatRaw(raw_byte, wire_error);
      debug_serial->printf(
          "IMU calibration: sys=%u gyro=%u accel=%u mag=%u (%lus "
          "elapsed) | raw I2C probe: %s (CALIB_STAT=0x%02X, "
          "wire_error=%u)\r\n",
          status.system, status.gyro, status.accel, status.mag,
          static_cast<unsigned long>((now - start) / 1000),
          i2c_ok ? "ok" : "FAILED", raw_byte, wire_error);
      last_log = now;
    }
    delay(20);
  }
  digitalWrite(red_led, LOW);
  if (debug_serial != nullptr) {
    debug_serial->printf("IMU calibration: timed out, nothing saved.\r\n");
  }
  return false;
}

}  // namespace imu_calibration_boot
