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

#include <STM32FreeRTOS.h>

#include "mavlink.h"
#include "mavlink_types.hpp"
#include "publisher_interface.hpp"

struct MavlinkJointStatePublisherConfig {
  QueueHandle_t& queue;
  uint32_t period_ms;
};

class MavlinkJointStatePublisher : public MavlinkPublisherInterface {
 public:
  explicit MavlinkJointStatePublisher(
      const MavlinkJointStatePublisherConfig& cfg)
      : cfg_(cfg) {}

  void publish(MavlinkNode& node) override {
    const uint32_t now = millis();
    if ((now - last_pub_ms_) < cfg_.period_ms && last_pub_ms_ != 0) return;
    last_pub_ms_ = now;

    JointStateStamped data;
    if (xQueueReceive(cfg_.queue, &data, 0) != pdPASS) return;

    const uint64_t t_us = static_cast<uint64_t>(data.timestamp_ns / 1000);
    // Wheel order: FL, FR, RL, RR — matches MotorArray and the dialect.
    float position[4] = {data.data.position[0], data.data.position[1],
                         data.data.position[2], data.data.position[3]};
    float velocity[4] = {data.data.velocity[0], data.data.velocity[1],
                         data.data.velocity[2], data.data.velocity[3]};
    float effort[4] = {data.data.effort[0], data.data.effort[1],
                       data.data.effort[2], data.data.effort[3]};

    mavlink_message_t m;
    mavlink_msg_rosbot_joint_state_pack(node.sysid(), node.compid(), &m, t_us,
                                        position, velocity, effort);
    node.sendMessage(m);
  }

 private:
  MavlinkJointStatePublisherConfig cfg_;
  uint32_t last_pub_ms_ = 0;
};
