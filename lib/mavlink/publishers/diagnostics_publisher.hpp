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

#include <cstring>

#include "mavlink.h"
#include "mcu_health.hpp"
#include "publisher_interface.hpp"
#include "time_base.hpp"

// ROSBOT_DIAGNOSTICS.flags bits (see rosbot.xml).
enum RosbotDiagFlag : uint8_t {
  ROSBOT_DIAG_BATTERY_LOW = 1,
  ROSBOT_DIAG_BATTERY_CRITICAL = 2,
  ROSBOT_DIAG_MOTOR_FAULT = 4,
  ROSBOT_DIAG_POWER_BOARD_OK = 8,
  ROSBOT_DIAG_ETHERNET_UP = 16,
  ROSBOT_DIAG_HAS_RANGES = 32,
  ROSBOT_DIAG_HAS_POWER_BOARD = 64,
  ROSBOT_DIAG_HAS_ETHERNET = 128,
};

// Board-specific part of ROSBOT_DIAGNOSTICS (sensors, power, link
// counters). The MCU fields are already set when it runs.
using DiagnosticsFill = void (*)(mavlink_rosbot_diagnostics_t& out);

struct MavlinkDiagnosticsPublisherConfig {
  DiagnosticsFill fill;
  uint32_t period_ms;
};

class MavlinkDiagnosticsPublisher : public MavlinkPublisherInterface {
 public:
  explicit MavlinkDiagnosticsPublisher(
      const MavlinkDiagnosticsPublisherConfig& cfg)
      : cfg_(cfg) {}

  void publish(MavlinkNode& node) override {
    const uint32_t now = time_boot_ms();
    if ((now - last_pub_ms_) < cfg_.period_ms && last_pub_ms_ != 0) return;
    last_pub_ms_ = now;

    const diagnostics::McuHealth h = diagnostics::sampleMcuHealth();
    mavlink_rosbot_diagnostics_t d{};
    d.uptime_ms = h.uptime_ms;
    d.reset_cause = h.reset_cause;
    d.cpu_idle = h.cpu_idle_permille;
    d.heap_free = h.heap_free;
    d.heap_min_free = h.heap_min_free;
    d.stack_min_free = static_cast<uint16_t>(
        h.stack_min_free > UINT16_MAX ? UINT16_MAX : h.stack_min_free);
    std::memcpy(d.stack_min_task, h.stack_min_task, sizeof(d.stack_min_task));
    d.fault_code = h.fault_code;
    d.fault_pc = h.fault_pc;
    std::memcpy(d.fault_task, h.fault_task, sizeof(d.fault_task));
    d.clock_source = h.clock_source;
    d.link_tx_dropped = node.txDropped();
    d.link_rx_errors = node.rxErrors();
    d.link_reconnects = static_cast<uint16_t>(node.reconnects());
    if (cfg_.fill != nullptr) cfg_.fill(d);

    mavlink_message_t m;
    mavlink_msg_rosbot_diagnostics_encode(node.sysid(), node.compid(), &m, &d);
    node.sendMessage(m);
  }

 private:
  MavlinkDiagnosticsPublisherConfig cfg_;
  uint32_t last_pub_ms_ = 0;
};
