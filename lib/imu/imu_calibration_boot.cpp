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

#include <cstdarg>
#include <cstdio>

#include "clock.hpp"
#include "time_base.hpp"

namespace imu_calibration_boot {

namespace {
constexpr uint32_t kLogIntervalMs = 1000;

void logLine(platform::Uart* a, platform::Uart* b, const char* fmt, ...) {
  if (a == nullptr && b == nullptr) return;
  char line[200];
  va_list args;
  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);
  if (a != nullptr) a->print(line);
  if (b != nullptr && b != a) b->print(line);
}
}  // namespace

bool run(ImuBno055& imu, platform::Pin red_led,
         etl::span<const platform::Pin> green_leds, ImuCalibrationOffsets& out,
         platform::Uart* debug_serial, platform::Uart* link_serial,
         uint32_t timeout_ms, uint32_t blink_ms) {
  const uint32_t start = time_boot_ms();
  uint32_t last_toggle = start;
  uint32_t last_log = start;
  bool led_on = false;

  logLine(debug_serial, link_serial,
          "IMU calibration: started, timeout=%lus. Move the robot - "
          "sit still (gyro), figure-8 in the air (mag), hold a few tilted "
          "rests >45deg apart (accel).\r\n",
          static_cast<unsigned long>(timeout_ms / 1000));

  while (time_boot_ms() - start < timeout_ms) {
    const ImuCalibrationStatus status = imu.getCalibrationStatus();
    if (status.fullyCalibrated()) {
      red_led.reset();
      for (const auto& led : green_leds) led.set();
      const bool captured = imu.captureCalibrationOffsets(out);
      logLine(debug_serial, link_serial,
              "IMU calibration: fully calibrated (sys=%u gyro=3 accel=3 "
              "mag=3), offsets %s.\r\n",
              status.system, captured ? "captured" : "capture FAILED");
      return captured;
    }
    const uint32_t now = time_boot_ms();
    if (now - last_toggle >= blink_ms) {
      led_on = !led_on;
      red_led.write(led_on);
      last_toggle = now;
    }
    if ((debug_serial != nullptr || link_serial != nullptr) &&
        now - last_log >= kLogIntervalMs) {
      uint8_t raw_byte = 0;
      uint8_t wire_error = 0;
      const bool i2c_ok = imu.probeCalibStatRaw(raw_byte, wire_error);
      logLine(debug_serial, link_serial,
              "IMU calibration: sys=%u gyro=%u accel=%u mag=%u (%lus "
              "elapsed) | raw I2C probe: %s (CALIB_STAT=0x%02X, "
              "wire_error=%u)\r\n",
              status.system, status.gyro, status.accel, status.mag,
              static_cast<unsigned long>((now - start) / 1000),
              i2c_ok ? "ok" : "FAILED", raw_byte, wire_error);
      last_log = now;
    }
    platform::clock::delayMs(20);
  }
  red_led.reset();
  logLine(debug_serial, link_serial,
          "IMU calibration: timed out, nothing saved.\r\n");
  return false;
}

}  // namespace imu_calibration_boot
