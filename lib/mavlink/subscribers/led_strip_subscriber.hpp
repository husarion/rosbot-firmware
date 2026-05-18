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

#include <cstring>

#include "led_strip.hpp"  // for LedFrameMsg
#include "mavlink.h"
#include "mavlink_node.hpp"
#include "subscriber_interface.hpp"

class LedStripSubscriber : public MavlinkSubscriberInterface {
 public:
  explicit LedStripSubscriber(QueueHandle_t& queue) : queue_(queue) {}

  uint32_t msgId() const override { return MAVLINK_MSG_ID_ROSBOT_LED_STRIP; }

  void onMessage(const mavlink_message_t& msg, MavlinkNode& /*node*/) override {
    mavlink_rosbot_led_strip_t s;
    mavlink_msg_rosbot_led_strip_decode(&msg, &s);

    LedFrameMsg frame{};
    frame.pixel_count = s.count;
    const uint32_t bytes =
        static_cast<uint32_t>(s.count) * 3u > sizeof(frame.rgb_data)
            ? sizeof(frame.rgb_data)
            : static_cast<uint32_t>(s.count) * 3u;
    std::memcpy(frame.rgb_data, s.rgb, bytes);
    xQueueOverwrite(queue_, &frame);
  }

 private:
  QueueHandle_t& queue_;
};
