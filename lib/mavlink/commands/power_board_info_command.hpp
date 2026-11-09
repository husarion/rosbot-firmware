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
#include <cstring>

#include "mavlink.h"
#include "mavlink_node.hpp"
#include "subscribers/subscriber_interface.hpp"

struct PowerBoardVersions {
  bool valid = false;
  char firmware_version[12] = {};
  char board_version[12] = {};
};

// lib/mavlink must not depend on lib/power_board, so the owner of the board
// hands in a getter.
using PowerBoardVersionsGetter = PowerBoardVersions (*)();

// MAV_CMD_USER_3 -> ROSBOT_POWER_BOARD.
class PowerBoardInfoCommand : public MavlinkSubscriberInterface {
 public:
  explicit PowerBoardInfoCommand(PowerBoardVersionsGetter getter)
      : getter_(getter) {}

  uint32_t msgId() const override { return MAVLINK_MSG_ID_COMMAND_LONG; }

  void onMessage(const mavlink_message_t& msg, MavlinkNode& node) override {
    mavlink_command_long_t cmd;
    mavlink_msg_command_long_decode(&msg, &cmd);
    if (cmd.command != MAV_CMD_USER_3) return;

    mavlink_message_t ack;
    mavlink_msg_command_ack_pack(
        node.sysid(), node.compid(), &ack, MAV_CMD_USER_3, MAV_RESULT_ACCEPTED,
        /*progress=*/255, /*result_param2=*/0, msg.sysid, msg.compid);
    node.sendMessage(ack);

    const PowerBoardVersions v = getter_();
    mavlink_message_t reply;
    mavlink_msg_rosbot_power_board_pack(node.sysid(), node.compid(), &reply,
                                        v.valid ? 1 : 0, v.firmware_version,
                                        v.board_version);
    node.sendMessage(reply);
  }

 private:
  PowerBoardVersionsGetter getter_;
};
