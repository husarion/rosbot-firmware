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

#include <std_srvs/srv/trigger.h>

#include <cstring>

#include "client_interface.hpp"

class TriggerClient : public ClientInterface {
 public:
  using Callback = void (*)(const void* response);

  TriggerClient(const char* service_name, Callback cb)
      : ClientInterface(service_name), cb_(cb) {
    memset(&req_, 0, sizeof(req_));
    memset(&res_, 0, sizeof(res_));
    res_.message.data = res_buf_;
    res_.message.size = 0;
    res_.message.capacity = sizeof(res_buf_);
  }

  rcl_ret_t init(rcl_node_t& node, rcl_allocator_t& allocator) override {
    (void)allocator;
    client_ = rcl_get_zero_initialized_client();
    return rclc_client_init_default(
        &client_, &node, ROSIDL_GET_SRV_TYPE_SUPPORT(std_srvs, srv, Trigger),
        service_name_);
  }

  rcl_ret_t send() override {
    int64_t seq;
    return rcl_send_request(&client_, &req_, &seq);
  }

  rcl_ret_t fini(rcl_node_t& node) override {
    return rcl_client_fini(&client_, &node);
  }

  rcl_client_t* clientHandle() override { return &client_; }
  void* responseMsg() override { return &res_; }
  rclc_client_callback_t responseCallback() override { return cb_; }

 private:
  rcl_client_t client_;
  Callback cb_;

  std_srvs__srv__Trigger_Request req_;
  std_srvs__srv__Trigger_Response res_;
  char res_buf_[64];
};
