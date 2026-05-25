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
#include <cstdio>
#include <cstring>

#include "mavlink.h"
#include "mavlink_node.hpp"
#include "subscribers/subscriber_interface.hpp"

class McuIdCommand : public MavlinkSubscriberInterface {
 public:
  uint32_t msgId() const override { return MAVLINK_MSG_ID_COMMAND_LONG; }

  void onMessage(const mavlink_message_t& msg, MavlinkNode& node) override {
    mavlink_command_long_t cmd;
    mavlink_msg_command_long_decode(&msg, &cmd);
    if (cmd.command != MAV_CMD_USER_1) return;

    // ACK first so the bridge can correlate retries.
    mavlink_message_t ack;
    mavlink_msg_command_ack_pack(
        node.sysid(), node.compid(), &ack, MAV_CMD_USER_1, MAV_RESULT_ACCEPTED,
        /*progress=*/255, /*result_param2=*/0, msg.sysid, msg.compid);
    node.sendMessage(ack);

    // STM32 UID @ 0x1FFF7A10 — 96 bits → 24 hex chars (no wire terminator),
    // +1 byte for snprintf's NUL on the final byte pair.
    char hex[25];
    const uint8_t* uid_bytes = reinterpret_cast<const uint8_t*>(0x1FFF7A10);
    for (int i = 0; i < 12; ++i) {
      snprintf(&hex[i * 2], 3, "%02X", uid_bytes[i]);
    }

    mavlink_message_t reply;
    mavlink_msg_rosbot_mcu_id_pack(node.sysid(), node.compid(), &reply, hex);
    node.sendMessage(reply);
  }
};
