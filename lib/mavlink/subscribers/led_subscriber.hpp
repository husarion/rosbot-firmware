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

#include "mavlink.h"
#include "mavlink_node.hpp"
#include "subscriber_interface.hpp"

struct PanelLedConfig {
  uint8_t pin;
  uint8_t bit_mask;
};

class PanelLedSubscriber : public MavlinkSubscriberInterface {
 public:
  PanelLedSubscriber(const PanelLedConfig* configs, uint8_t count)
      : configs_(configs), count_(count) {}

  uint32_t msgId() const override { return MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS; }

  void onMessage(const mavlink_message_t& msg, MavlinkNode& /*node*/) override {
    mavlink_rosbot_panel_leds_t leds;
    mavlink_msg_rosbot_panel_leds_decode(&msg, &leds);
    for (uint8_t i = 0; i < count_; ++i) {
      const bool on = (leds.mask & configs_[i].bit_mask) != 0;
      digitalWrite(configs_[i].pin, on ? HIGH : LOW);
    }
  }

 private:
  const PanelLedConfig* configs_;
  uint8_t count_;
};
