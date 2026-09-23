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

#include <Arduino.h>

#include "imu_calibration_runtime.hpp"
#include "imu_interface.hpp"
#include "mavlink.h"
#include "persistent_config.hpp"
#include "publisher_interface.hpp"

struct MavlinkImuCalibrationPublisherConfig {
  uint32_t period_ms;
};

class MavlinkImuCalibrationPublisher : public MavlinkPublisherInterface {
 public:
  explicit MavlinkImuCalibrationPublisher(
      const MavlinkImuCalibrationPublisherConfig& cfg)
      : cfg_(cfg) {}

  void publish(MavlinkNode& node) override {
    const uint32_t now = millis();
    if ((now - last_pub_ms_) < cfg_.period_ms && last_pub_ms_ != 0) return;
    last_pub_ms_ = now;

    ImuCalibrationStatus status{};
    if (g_imu == nullptr || !g_imu->calibrationStatus(status)) return;

    const auto& control = imu_calibration::g_control;
    mavlink_message_t m;
    mavlink_msg_rosbot_imu_calibration_pack(
        node.sysid(), node.compid(), &m, status.system, status.gyro,
        status.accel, status.mag, control.state.load(), control.save_seq.load(),
        persistent_config::hasImuCalibration() ? 1 : 0,
        imu_calibration::sessionActive(now) ? 1 : 0);
    node.sendMessage(m);
  }

 private:
  MavlinkImuCalibrationPublisherConfig cfg_;
  uint32_t last_pub_ms_ = 0;
};
