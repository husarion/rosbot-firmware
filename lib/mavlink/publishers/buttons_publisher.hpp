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
#include <etl/span.h>

#include "mavlink.h"
#include "pin.hpp"
#include "publisher_interface.hpp"
#include "time_base.hpp"

struct MavlinkButtonsPublisherConfig {
  etl::span<const platform::Pin> pins;  // active-low
  uint32_t period_ms;
};

class MavlinkButtonsPublisher : public MavlinkPublisherInterface {
 public:
  explicit MavlinkButtonsPublisher(const MavlinkButtonsPublisherConfig& cfg)
      : cfg_(cfg) {}

  void publish(MavlinkNode& node) override {
    const uint32_t now = time_boot_ms();
    if ((now - last_pub_ms_) < cfg_.period_ms && last_pub_ms_ != 0) return;
    last_pub_ms_ = now;

    uint8_t mask = 0;
    for (size_t i = 0; i < cfg_.pins.size(); ++i) {
      if (!cfg_.pins[i].read()) mask |= (1u << i);
    }

    mavlink_message_t m;
    const uint64_t t_us = static_cast<uint64_t>(MavlinkNode::timeBootUs());
    mavlink_msg_rosbot_buttons_pack(node.sysid(), node.compid(), &m, t_us,
                                    mask);
    node.sendMessage(m);
  }

 private:
  MavlinkButtonsPublisherConfig cfg_;
  uint32_t last_pub_ms_ = 0;
};
