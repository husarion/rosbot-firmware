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

#include <HardwareSerial.h>
#include <STM32FreeRTOS.h>

#include <cstddef>
#include <cstdint>

#include "mavlink.h"
#include "robotics_link.hpp"
#include "transport/mavlink_transport_interface.hpp"

class MavlinkPublisherInterface;
class MavlinkSubscriberInterface;

struct MavlinkNodeConfig {
  uint8_t sysid = 1;
  uint8_t compid = MAV_COMP_ID_AUTOPILOT1;
  uint8_t mav_type = MAV_TYPE_GROUND_ROVER;
  uint8_t autopilot = MAV_AUTOPILOT_GENERIC;
  // Bridge gates the CONNECTED transition on seeing this banner.
  const char* boot_banner = "rosbot mavlink";
  uint32_t heartbeat_period_ms = 1000;
  uint32_t timesync_period_ms = 2000;
  uint32_t timesync_active_period_ms = 200;
  uint32_t peer_timeout_ms = 3000;
  MavlinkPublisherInterface** publishers = nullptr;
  size_t pub_count = 0;
  MavlinkSubscriberInterface** subscribers = nullptr;
  size_t sub_count = 0;
};

class MavlinkNode : public RoboticsLink {
 public:
  enum State : uint8_t { WAITING, AWAIT_TIMESYNC, CONNECTED, DISCONNECTED };

  MavlinkNode(MavlinkTransport& transport, const MavlinkNodeConfig& cfg)
      : transport_(transport), cfg_(cfg) {}

  bool begin();

  void loop() override;
  bool isConnected() const override { return state_ == CONNECTED; }
  void setNamespace(const char* ns) override { ns_ = ns; }
  void setDiagnosticSerial(HardwareSerial* serial) override {
    diag_serial_ = serial;
  }

  bool sendMessage(mavlink_message_t& msg);
  static uint64_t timeBootUs();
  void log(uint8_t severity, const char* fmt, ...);

  uint8_t sysid() const { return cfg_.sysid; }
  uint8_t compid() const { return cfg_.compid; }
  State state() const { return state_; }
  const char* nsName() const { return ns_; }

 private:
  void emitHeartbeatIfDue(uint32_t now_ms);
  void emitBootBannerIfDue(uint32_t now_ms);
  void emitTimesyncIfDue(uint32_t now_ms);
  void drainRx();
  void dispatchMessage(const mavlink_message_t& msg);

  MavlinkTransport& transport_;
  MavlinkNodeConfig cfg_;
  HardwareSerial* diag_serial_ = nullptr;
  const char* ns_ = "";
  mavlink_status_t rx_status_ = {};
  mavlink_message_t rx_msg_ = {};
  bool transport_open_ = false;
  bool boot_banner_sent_ = false;
  uint32_t last_heartbeat_ms_ = 0;
  uint32_t last_timesync_ms_ = 0;
  uint32_t last_peer_heartbeat_ms_ = 0;
  uint32_t last_boot_banner_ms_ = 0;
  bool peer_seen_ = false;
  uint8_t boot_banner_attempts_ = 0;
  State state_ = WAITING;
  SemaphoreHandle_t tx_mutex_ = nullptr;
};

extern MavlinkNode g_mavlink_node;
