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

#include "ros_node.hpp"

#include "transport/lwip_udp_transport.hpp"
#include "transport/serial_transport.hpp"

RosNode* RosNode::instance_ = nullptr;

bool RosNode::pingAgent() {
  return rmw_uros_ping_agent(cfg_.ping_timeout_ms, cfg_.ping_attempts) ==
         RMW_RET_OK;
}

bool RosNode::createEntities() {
  allocator_ = rcl_get_default_allocator();
  executor_ = rclc_executor_get_zero_initialized_executor();
  init_options_ = rcl_get_zero_initialized_init_options();
  node_ = rcl_get_zero_initialized_node();
  support_ = rclc_support_t{};
  timer_ = rcl_get_zero_initialized_timer();

  RC_CHECK(rcl_init_options_init(&init_options_, allocator_));
  RC_CHECK(rcl_init_options_set_domain_id(&init_options_, cfg_.domain_id));
  RC_CHECK(rclc_support_init_with_options(&support_, 0, NULL, &init_options_,
                                          &allocator_));
  RC_CHECK(rcl_init_options_fini(&init_options_));

  RC_CHECK(rclc_node_init_default(&node_, cfg_.node_name, ns_, &support_));

  // ── Publishers ───────────────────────────────────────────
  for (size_t i = 0; i < cfg_.pub_count; ++i) {
    RC_CHECK(cfg_.publishers[i]->init(node_, allocator_));
  }

  // ── Subscriptions ────────────────────────────────────────
  for (size_t i = 0; i < cfg_.sub_count; ++i) {
    auto& s = cfg_.subscriptions[i];
    s.sub = rcl_get_zero_initialized_subscription();
    if (s.best_effort) {
      RC_CHECK(rclc_subscription_init_best_effort(
          &s.sub, &node_, s.type_support, s.topic_name));
    } else {
      RC_CHECK(rclc_subscription_init_default(&s.sub, &node_, s.type_support,
                                              s.topic_name));
    }
  }

  // ── Service Clients ─────────────────────────────────────
  for (size_t i = 0; i < cfg_.client_count; ++i) {
    RC_CHECK(cfg_.clients[i]->init(node_, allocator_));
  }

  // ── Services Server ───────────────────────────────────
  for (size_t i = 0; i < cfg_.srv_count; ++i) {
    auto& s = cfg_.services[i];
    s.srv = rcl_get_zero_initialized_service();  // ← KLUCZOWE
    RC_CHECK(rclc_service_init_default(&s.srv, &node_, s.type_support,
                                       s.service_name));
  }

  // ── Timer ───────────────────────────────────────────────
  RC_CHECK(rclc_timer_init_default(&timer_, &support_,
                                   RCL_MS_TO_NS(cfg_.timer_ms), timerCallback));

  // ── Executor ─────────────────────────────────────────────
  size_t exec_count = 1 + cfg_.sub_count + cfg_.srv_count +
                      cfg_.client_count;  // timer + subs + srvs + clients
  RC_CHECK(rclc_executor_init(&executor_, &support_.context, exec_count,
                              &allocator_));

  for (size_t i = 0; i < cfg_.sub_count; ++i) {
    auto& s = cfg_.subscriptions[i];
    RC_CHECK(rclc_executor_add_subscription(&executor_, &s.sub, s.msg,
                                            s.callback, ON_NEW_DATA));
  }
  for (size_t i = 0; i < cfg_.client_count; ++i) {
    auto* c = cfg_.clients[i];
    RC_CHECK(rclc_executor_add_client(&executor_, c->clientHandle(),
                                      c->responseMsg(), c->responseCallback()));
  }
  for (size_t i = 0; i < cfg_.srv_count; ++i) {
    auto& s = cfg_.services[i];
    RC_CHECK(rclc_executor_add_service(&executor_, &s.srv, s.request,
                                       s.response, s.callback));
  }
  RC_CHECK(rclc_executor_add_timer(&executor_, &timer_));

  RC_CHECK(rmw_uros_sync_session(1000));
  instance_ = this;
  return true;
}

void RosNode::destroyEntities() {
  auto* ctx = rcl_context_get_rmw_context(&support_.context);
  RC_CHECK(rmw_uros_set_context_entity_destroy_session_timeout(ctx, 0));

  RC_CHECK(rcl_timer_fini(&timer_));
  for (uint8_t i = 0; i < cfg_.pub_count; ++i) {
    RC_CHECK(cfg_.publishers[i]->fini(node_));
  }
  for (uint8_t i = 0; i < cfg_.sub_count; ++i) {
    RC_CHECK(rcl_subscription_fini(&cfg_.subscriptions[i].sub, &node_));
  }
  for (uint8_t i = 0; i < cfg_.srv_count; ++i) {
    RC_CHECK(rcl_service_fini(&cfg_.services[i].srv, &node_));
  }
  for (uint8_t i = 0; i < cfg_.client_count; ++i) {
    RC_CHECK(cfg_.clients[i]->fini(node_));
  }

  RC_CHECK(rclc_executor_fini(&executor_));
  RC_CHECK(rcl_node_fini(&node_));
  RC_CHECK(rclc_support_fini(&support_));
  instance_ = nullptr;
}

void RosNode::loop() {
  const TickType_t now = xTaskGetTickCount();
  if ((now - last_ping_) >= pdMS_TO_TICKS(cfg_.ping_watchdog_ms)) {
    last_ping_ = now;
    ping_ = pingAgent();
  }

  switch (state_) {
    case WAITING:
      if (ping_) state_ = AGENT_AVAILABLE;
      break;
    case AGENT_AVAILABLE:
      if (createEntities())
        state_ = CONNECTED;
      else {
        destroyEntities();
        state_ = WAITING;
      }
      break;
    case CONNECTED:
      if (!ping_) {
        state_ = DISCONNECTED;
      } else {
        spin();
      }
      break;
    case DISCONNECTED:
      destroyEntities();
      state_ = WAITING;
      break;
    default:
      break;
  }
}

void RosNode::publish() {
  if (state_ != CONNECTED) return;
  for (uint8_t i = 0; i < cfg_.pub_count; ++i) cfg_.publishers[i]->publish();
}

void RosNode::spin() {
  if (state_ != CONNECTED) return;
  RC_CHECK(
      rclc_executor_spin_some(&executor_, RCL_MS_TO_NS(cfg_.spin_time_ms)));
}

void RosNode::ethernetTransportInit(IPAddress agent_ip, uint16_t agent_port) {
  static struct micro_ros_agent_locator locator;

  locator.address = agent_ip;
  locator.port = agent_port;

  RC_CHECK(rmw_uros_set_custom_transport(
      false, (void*)&locator, lwip_udp_transport_open, lwip_udp_transport_close,
      lwip_udp_transport_write, lwip_udp_transport_read));
}

void RosNode::serialTransportInit(const SerialConfig& config) {
  RC_CHECK(rmw_uros_set_custom_transport(
      /* Enable XRCE framing */
      true,
      /* Arguments for callbacks */
      (void*)&config, serial_transport_open, serial_transport_close,
      serial_transport_write, serial_transport_read));
}
