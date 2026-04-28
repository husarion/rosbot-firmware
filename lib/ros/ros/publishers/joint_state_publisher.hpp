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

#include <micro_ros_utilities/string_utilities.h>
#include <rosidl_runtime_c/string_functions.h>
#include <sensor_msgs/msg/joint_state.h>

#include "motor_array.hpp"
#include "publisher_interface.hpp"

struct JointStateStamped {
  MotorsData data;
  int64_t timestamp_ns;
};

struct JointStatePublisherConfig {
  const char* topic;
  QueueHandle_t& queue;
  const char* frame_id;
};

class JointStatePublisher : public PublisherInterface {
 public:
  explicit JointStatePublisher(JointStatePublisherConfig cfg)
      : PublisherInterface(cfg.topic), cfg_(cfg) {}

  rcl_ret_t init(rcl_node_t& node, rcl_allocator_t& allocator) override {
    initMsg(allocator);
    return rclc_publisher_init_best_effort(
        &pub_, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
        topic_);
  }

  rcl_ret_t publish() override {
    if (xQueueReceive(cfg_.queue, &data_, 0) != pdPASS) return RCL_RET_OK;
    fillMsg(data_);
    return rcl_publish(&pub_, &msg_, NULL);
  }

  rcl_ret_t fini(rcl_node_t& node) override {
    sensor_msgs__msg__JointState__fini(&msg_);
    return rcl_publisher_fini(&pub_, &node);
  }

 private:
  rcl_publisher_t pub_;
  sensor_msgs__msg__JointState msg_;
  JointStateStamped data_;
  JointStatePublisherConfig cfg_;

  void initMsg(rcl_allocator_t& allocator) {
    sensor_msgs__msg__JointState__init(&msg_);

    msg_.header.frame_id =
        micro_ros_string_utilities_set(msg_.header.frame_id, cfg_.frame_id);

    const uint8_t n = g_motors.count();

    rosidl_runtime_c__String__Sequence__init(&msg_.name, n);
    rosidl_runtime_c__double__Sequence__init(&msg_.position, n);
    rosidl_runtime_c__double__Sequence__init(&msg_.velocity, n);
    rosidl_runtime_c__double__Sequence__init(&msg_.effort, n);

    for (uint8_t i = 0; i < n; ++i)
      msg_.name.data[i] = micro_ros_string_utilities_set(msg_.name.data[i],
                                                         g_motors[i]->name());
  }

  void fillMsg(const JointStateStamped& d) {
    msg_.header.stamp.sec = d.timestamp_ns / 1000000000LL;
    msg_.header.stamp.nanosec = d.timestamp_ns % 1000000000LL;

    const uint8_t n = g_motors.count();
    for (uint8_t i = 0; i < n; ++i) {
      msg_.position.data[i] = d.data.position[i];
      msg_.velocity.data[i] = d.data.velocity[i];
      msg_.effort.data[i] = d.data.effort[i];
    }
  }
};
