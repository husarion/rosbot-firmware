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

#include <limits>

#include "mavlink.h"
#include "mavlink_types.hpp"
#include "publisher_interface.hpp"

struct MavlinkBatteryPublisherConfig {
  QueueHandle_t& queue;
  uint8_t num_cells;
  uint32_t period_ms;
};

class MavlinkBatteryPublisher : public MavlinkPublisherInterface {
 public:
  explicit MavlinkBatteryPublisher(const MavlinkBatteryPublisherConfig& cfg)
      : cfg_(cfg) {}

  void publish(MavlinkNode& node) override {
    const uint32_t now = millis();
    if ((now - last_pub_ms_) < cfg_.period_ms && last_pub_ms_ != 0) return;
    last_pub_ms_ = now;

    BatteryStamped data;
    if (xQueueReceive(cfg_.queue, &data, 0) != pdPASS) return;

    uint16_t voltages[10];
    for (uint8_t i = 0; i < 10; ++i) voltages[i] = UINT16_MAX;

    // No per-cell instrumentation — split the pack voltage evenly so any
    // consumer that sums voltages still gets the right total.
    const float per_cell_v =
        (cfg_.num_cells > 0) ? data.data.voltage / cfg_.num_cells : 0.0f;
    for (uint8_t i = 0; i < cfg_.num_cells && i < 10; ++i) {
      voltages[i] = static_cast<uint16_t>(per_cell_v * 1000.0f);
    }

    const int16_t current_cA =
        (data.data.current >= 0.0f)
            ? static_cast<int16_t>(data.data.current * 100.0f)
            : -1;
    int8_t percent = -1;
    if (!isnan(data.data.percentage)) {
      percent = static_cast<int8_t>(data.data.percentage * 100.0f);
    }

    uint16_t voltages_ext[4] = {0, 0, 0, 0};
    mavlink_message_t m;
    mavlink_msg_battery_status_pack(
        node.sysid(), node.compid(), &m, /*id=*/0, MAV_BATTERY_FUNCTION_ALL,
        MAV_BATTERY_TYPE_LION,
        /*temperature (cdegC)=*/INT16_MAX, voltages, current_cA,
        /*current_consumed=*/-1, /*energy_consumed=*/-1, percent,
        /*time_remaining=*/0, MAV_BATTERY_CHARGE_STATE_UNDEFINED, voltages_ext,
        MAV_BATTERY_MODE_UNKNOWN, /*fault_bitmask=*/0);
    node.sendMessage(m);
  }

 private:
  MavlinkBatteryPublisherConfig cfg_;
  uint32_t last_pub_ms_ = 0;
};
