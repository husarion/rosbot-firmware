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

#include <cstdint>

#include "imu_calibration_runtime.hpp"
#include "imu_interface.hpp"
#include "mavlink.h"
#include "mavlink_node.hpp"
#include "subscribers/subscriber_interface.hpp"

// MAV_CMD_USER_2, param1 = imu_calibration::Action. kSave persists the
// offsets the BNO055 is using right now; its ACK only says whether the
// request was taken, the outcome follows in ROSBOT_IMU_CALIBRATION (state +
// a bumped save_seq), because the save runs on the IMU task, which owns the
// bus. kStart/kStop only drive the on-robot LED feedback.
class ImuCalibrationCommand : public MavlinkSubscriberInterface {
 public:
  uint32_t msgId() const override { return MAVLINK_MSG_ID_COMMAND_LONG; }

  void onMessage(const mavlink_message_t& msg, MavlinkNode& node) override {
    mavlink_command_long_t cmd;
    mavlink_msg_command_long_decode(&msg, &cmd);
    if (cmd.command != MAV_CMD_USER_2) return;

    const auto action =
        static_cast<imu_calibration::Action>(static_cast<uint8_t>(cmd.param1));
    if (action == imu_calibration::Action::kStart ||
        action == imu_calibration::Action::kStop) {
      if (action == imu_calibration::Action::kStart) {
        imu_calibration::startSession(millis());
      } else {
        imu_calibration::stopSession();
      }
      sendAck(node, msg, MAV_RESULT_ACCEPTED, 0);
      return;
    }
    if (action != imu_calibration::Action::kSave) {
      sendAck(node, msg, MAV_RESULT_UNSUPPORTED, 0);
      return;
    }

    auto& control = imu_calibration::g_control;
    ImuCalibrationStatus status{};
    const bool calibrated = g_imu != nullptr &&
                            g_imu->calibrationStatus(status) &&
                            status.fullyCalibrated();
    const bool busy =
        control.save_requested.load() ||
        control.state.load() ==
            static_cast<uint8_t>(imu_calibration::SaveState::kSaving);

    uint8_t result = MAV_RESULT_ACCEPTED;
    if (!calibrated || busy) {
      result = MAV_RESULT_TEMPORARILY_REJECTED;
    } else {
      control.save_requested.store(true);
    }

    sendAck(node, msg, result, calibrated ? 0 : 1);
  }

 private:
  static void sendAck(MavlinkNode& node, const mavlink_message_t& msg,
                      uint8_t result, int32_t detail) {
    mavlink_message_t ack;
    mavlink_msg_command_ack_pack(node.sysid(), node.compid(), &ack,
                                 MAV_CMD_USER_2, result, /*progress=*/255,
                                 detail, msg.sysid, msg.compid);
    node.sendMessage(ack);
  }
};
