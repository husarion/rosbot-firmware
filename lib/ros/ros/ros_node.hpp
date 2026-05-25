// Copyright 2022 Husarion sp. z o.o.
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
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <rmw_microros/rmw_microros.h>

#include "clients/client_interface.hpp"
#include "communication_manager.hpp"
#include "publishers/publisher_interface.hpp"
#include "robotics_link.hpp"
#include "types.hpp"

#define RC_CHECK(fn)                                                  \
  {                                                                   \
    rcl_ret_t rc = fn;                                                \
    if (rc != RCL_RET_OK) {                                           \
      log("[ERROR] %s:%d failed with rc=%d", __FILE__, __LINE__, rc); \
    }                                                                 \
  }

struct RosNodeConfig {
  const char* node_name;
  uint8_t domain_id;  // 255 = inherit from agent

  PublisherInterface** publishers = nullptr;
  size_t pub_count = 0;

  SubscriptionEntry* subscriptions = nullptr;
  size_t sub_count = 0;

  ClientInterface** clients = nullptr;
  size_t client_count = 0;

  ServiceEntry* services = nullptr;
  size_t srv_count = 0;

  uint32_t spin_time_ms = 10;
  uint32_t timer_ms = 10;

  uint16_t ping_watchdog_ms = 200;
  uint8_t ping_attempts = 1;
  uint16_t ping_timeout_ms = 100;
};

class RosNode : public RoboticsLink {
 public:
  enum State : uint8_t { WAITING, AGENT_AVAILABLE, CONNECTED, DISCONNECTED };

  RosNode() = default;
  explicit RosNode(const RosNodeConfig& cfg) : cfg_(cfg) {}
  ~RosNode() { destroyEntities(); }

  void serialTransportInit(const SerialConfig& serial);
  void ethernetTransportInit(IPAddress agent_ip, uint16_t agent_port);
  bool pingAgent();

  /// State machine + spin.
  void loop() override;
  void publish();

  static void timerCallback(rcl_timer_t* timer, int64_t last_call_time) {
    (void)timer;
    (void)last_call_time;
    if (instance_) {
      instance_->publish();
    }
  }

  State state() const { return state_; }
  bool isConnected() const override { return state_ == CONNECTED; }

  void setNamespace(const char* ns) override { ns_ = ns; };
  void setDiagnosticSerial(HardwareSerial* serial) override {
    serial_ = serial;
  }

 private:
  bool createEntities();
  void destroyEntities();
  void spin();
  template <typename... Args>
  void log(const char* fmt, Args... args) {
    if (!serial_) return;
    char buf[128];
    snprintf(buf, sizeof(buf), fmt, args...);
    serial_->println(buf);
  }

  RosNodeConfig cfg_ = {};
  State state_ = WAITING;
  const char* ns_ = {};
  HardwareSerial* serial_ = nullptr;
  TickType_t last_ping_ = 0;
  bool ping_ = false;

  // ROS2 internals
  rcl_allocator_t allocator_ = {};
  rclc_executor_t executor_ = {};
  rcl_init_options_t init_options_ = {};
  rcl_node_t node_ = {};
  rclc_support_t support_ = {};
  rcl_timer_t timer_ = {};
  static RosNode* instance_;  // wskaźnik na aktywną instancję
};

/// Global ROS node — defined in board-specific ros_entities.cpp
extern RosNode g_ros_node;
