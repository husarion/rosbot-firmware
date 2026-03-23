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

  // ── Executor ─────────────────────────────────────────────
  size_t exec_count = cfg_.sub_count + cfg_.srv_count + cfg_.client_count;
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

  RC_CHECK(rmw_uros_sync_session(1000));
  return true;
}

void RosNode::destroyEntities() {
  auto* ctx = rcl_context_get_rmw_context(&support_.context);
  RC_CHECK(rmw_uros_set_context_entity_destroy_session_timeout(ctx, 0));

  RC_CHECK(rclc_executor_fini(&executor_));

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

  RC_CHECK(rcl_node_fini(&node_));
  RC_CHECK(rclc_support_fini(&support_));
}

void RosNode::loop() {
  switch (state_) {
    case WAITING:
      if (pingAgent()) state_ = AGENT_AVAILABLE;
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
      if (!pingAgent()) state_ = DISCONNECTED;
      break;
    case DISCONNECTED:
      destroyEntities();
      state_ = WAITING;
      break;
  }
}

void RosNode::publishLoop() {
  if (state_ != CONNECTED) return;
  for (uint8_t i = 0; i < cfg_.pub_count; ++i) cfg_.publishers[i]->publish();
  RC_CHECK(
      rclc_executor_spin_some(&executor_, RCL_MS_TO_NS(cfg_.spin_time_ms)));
}

void RosNode::ethernetTransportInit(IPAddress agent_ip, uint16_t agent_port) {
  static struct micro_ros_agent_locator locator;

  locator.address = agent_ip;
  locator.port = agent_port;

  RC_CHECK(rmw_uros_set_custom_transport(
      false, (void*)&locator, arduino_native_ethernet_udp_transport_open,
      arduino_native_ethernet_udp_transport_close,
      arduino_native_ethernet_udp_transport_write,
      arduino_native_ethernet_udp_transport_read));
}

void RosNode::serialTransportInit(const SerialConfig& config) {
  RC_CHECK(rmw_uros_set_custom_transport(
      /* Enable XRCE framing */
      true,
      /* Arguments for callbacks - pass config pointer */
      (void*)&config,

      /* Open transport callback */
      [](struct uxrCustomTransport* transport) -> bool {
        const SerialConfig* cfg = (const SerialConfig*)transport->args;
        cfg->serial->setRx(cfg->rxPin);
        cfg->serial->setTx(cfg->txPin);
        cfg->serial->setTimeout(cfg->timeout_ms);
        cfg->serial->begin(cfg->baudrate);
        return cfg->serial->operator bool();
      },

      /* Close transport callback */
      [](struct uxrCustomTransport* transport) -> bool {
        const SerialConfig* cfg = (const SerialConfig*)transport->args;
        cfg->serial->end();
        return true;
      },

      /* Write transport callback */
      [](struct uxrCustomTransport* transport, const uint8_t* buf, size_t len,
         uint8_t* errcode) -> unsigned int {
        const SerialConfig* cfg = (const SerialConfig*)transport->args;
        return cfg->serial->write(buf, len);
      },

      /* Read transport callback */
      [](struct uxrCustomTransport* transport, uint8_t* buf, size_t len,
         int timeout, uint8_t* errcode) -> unsigned int {
        const SerialConfig* cfg = (const SerialConfig*)transport->args;
        cfg->serial->setTimeout(timeout);
        return cfg->serial->readBytes((char*)buf, len);
      }));
}
