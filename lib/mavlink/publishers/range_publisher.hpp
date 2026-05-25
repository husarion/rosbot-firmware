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

#ifdef ROSBOT

struct MavlinkRangePublisherConfig {
  QueueHandle_t& queue;
  float min_range_m;
  float max_range_m;
  uint32_t period_ms;
};

class MavlinkRangePublisher : public MavlinkPublisherInterface {
 public:
  explicit MavlinkRangePublisher(const MavlinkRangePublisherConfig& cfg)
      : cfg_(cfg) {}

  void publish(MavlinkNode& node) override {
    const uint32_t now = millis();
    if ((now - last_pub_ms_) < cfg_.period_ms && last_pub_ms_ != 0) return;
    last_pub_ms_ = now;

    RangesStamped data;
    if (xQueueReceive(cfg_.queue, &data, 0) != pdPASS) return;

    // time_boot_ms truncates to uint32_t — monotonic over any 49-day window.
    const uint32_t t_ms = static_cast<uint32_t>(data.timestamp_ns / 1000000);
    const uint16_t min_cm = static_cast<uint16_t>(cfg_.min_range_m * 100.0f);
    const uint16_t max_cm = static_cast<uint16_t>(cfg_.max_range_m * 100.0f);

    float quaternion[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for (uint8_t i = 0; i < data.data.count && i < 4; ++i) {
      const uint16_t current_cm =
          static_cast<uint16_t>(data.data.range[i] * 100.0f);
      mavlink_message_t m;
      mavlink_msg_distance_sensor_pack(
          node.sysid(), node.compid(), &m, t_ms, min_cm, max_cm, current_cm,
          MAV_DISTANCE_SENSOR_LASER, /*id=*/i, MAV_SENSOR_ROTATION_NONE,
          /*covariance=*/UINT8_MAX,
          /*horizontal_fov=*/0.0f,
          /*vertical_fov=*/0.0f, quaternion,
          /*signal_quality=*/0);
      node.sendMessage(m);
    }
  }

 private:
  MavlinkRangePublisherConfig cfg_;
  uint32_t last_pub_ms_ = 0;
};

#endif  // ROSBOT
