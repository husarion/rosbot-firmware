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

#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>

class ClientInterface {
 public:
  explicit ClientInterface(const char* service_name)
      : service_name_(service_name) {}

  /// Initialize client on the given node.
  virtual rcl_ret_t init(rcl_node_t& node, rcl_allocator_t& allocator) = 0;

  /// Send request. Returns true if rcl_send_request succeeded.
  virtual rcl_ret_t send() = 0;

  /// Cleanup. Called during destroyEntities().
  virtual rcl_ret_t fini(rcl_node_t& node) = 0;

  /// Executor needs these to register the client.
  virtual rcl_client_t* clientHandle() = 0;
  virtual void* responseMsg() = 0;
  virtual rclc_client_callback_t responseCallback() = 0;

  virtual const char* serviceName() const { return service_name_; }

 protected:
  const char* service_name_;
};
