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

#include "mavlink.h"
#include "mavlink_node.hpp"
#include "motor_array.hpp"
#include "subscriber_interface.hpp"

class WheelCmdSubscriber : public MavlinkSubscriberInterface {
 public:
  explicit WheelCmdSubscriber(MotorArray& motors) : motors_(motors) {}
  uint32_t msgId() const override {
    return MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS;
  }
  void onMessage(const mavlink_message_t& msg, MavlinkNode& /*node*/) override {
    mavlink_rosbot_wheel_setpoints_t s;
    mavlink_msg_rosbot_wheel_setpoints_decode(&msg, &s);
    // setVelocities feeds the 500 ms motor watchdog; bridge dropout
    // auto-stops the wheels.
    motors_.setVelocities(s.velocity);
  }

 private:
  MotorArray& motors_;
};
