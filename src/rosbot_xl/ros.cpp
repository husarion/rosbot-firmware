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

#include <string>
#include <vector>

#include "ros/ros_node.hpp"

/*===== ROS MSGS TYPES =====*/
#include <sensor_msgs/msg/image.h>
#include <std_msgs/msg/bool.h>
#include <std_msgs/msg/float32_multi_array.h>
#include <std_msgs/msg/string.h>
#include <std_srvs/srv/trigger.h>

#include "config.hpp"
#include "led_strip.hpp"
#include "motor_array.hpp"
#include "ros/clients/trigger_client.hpp"
#include "ros/publishers/battery_publisher.hpp"
#include "ros/publishers/buttons_publisher.hpp"
#include "ros/publishers/imu_publisher.hpp"
#include "ros/publishers/joint_state_publisher.hpp"
#include "ros/subscribers/led_subscriber.hpp"

// Pub configs kept here, not config.hpp — see src/rosbot/ros.cpp.
static constexpr BatteryPublisherConfig battery_pub_config = {
    .topic = "battery",
    .queue = battery_queue,
    .frame_id = "base_link",
    .design_capacity = BATTERY_DESIGN_CAPACITY,
    .num_cells = BATTERY_NUM_CELLS,
};

static constexpr ButtonsPublisherConfig buttons_pub_config = {
    .topic = "buttons",
    .pins = buttons_pins,
    .num_buttons = sizeof(buttons_pins) / sizeof(buttons_pins[0]),
};

static constexpr ImuPublisherConfig imu_pub_config = {
    .topic = "_imu/data",
    .queue = imu_queue,
    .frame_id = "imu_link",
};

static constexpr JointStatePublisherConfig joint_state_pub_config = {
    .topic = "_motors/feedback",
    .queue = joint_state_queue,
    .frame_id = "base_link",
};

// PUBLISHERS
static BatteryPublisher s_battery_pub(battery_pub_config);
static ButtonsPublisher s_buttons_pub(buttons_pub_config);
static ImuPublisher s_imu_pub(imu_pub_config);
static JointStatePublisher s_joint_pub(joint_state_pub_config);

static std::vector<PublisherInterface*> publishers = {
    &s_battery_pub, &s_buttons_pub, &s_imu_pub, &s_joint_pub};
uint8_t pub_count = static_cast<uint8_t>(publishers.size());

// SUBSCRIBERS

// Rear Leds subscriber
const LedConfig s_led_configs[] = {
    {.pin = GRN_LED, .bit_mask = 0x01},
};

LedState s_leds_state = {
    .msg = {},
    .config = s_led_configs,
    .config_count = sizeof(s_led_configs) / sizeof(s_led_configs[0]),
};

SubscriptionEntry leds_sub = {
    .sub = rcl_get_zero_initialized_subscription(),
    .msg = &s_leds_state.msg,
    .type_support = ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt8),
    .topic_name = "leds",
    .callback = ledsCallback,
    .best_effort = true,
};

// LED strip subscriber
constexpr uint8_t MAX_ENCODING_SIZE = 16;

static sensor_msgs__msg__Image s_img_msg = {
    .height = 1,
    .width = MAX_NUM_LEDS,
    .encoding =
        {
            .data = new char[MAX_ENCODING_SIZE](),
            .size = 0,
            .capacity = MAX_ENCODING_SIZE,
        },
    .is_bigendian = false,
    .step = MAX_NUM_LEDS * 3,
    .data =
        {
            .data = new uint8_t[MAX_NUM_LEDS * 3](),
            .size = 0,
            .capacity = MAX_NUM_LEDS * 3,
        },
};

void ledStripCallback(const void* msg_in) {
  const sensor_msgs__msg__Image* img =
      reinterpret_cast<const sensor_msgs__msg__Image*>(msg_in);

  if (img->height != 1) return;
  if (strcmp(img->encoding.data, "rgb8") != 0) return;

  LedFrameMsg frame;
  frame.pixel_count = img->width;

  uint32_t bytes_to_copy = frame.pixel_count * 3;
  if (bytes_to_copy > sizeof(frame.rgb_data)) {
    bytes_to_copy = sizeof(frame.rgb_data);
    frame.pixel_count = MAX_NUM_LEDS;
  }

  memcpy(frame.rgb_data, img->data.data, bytes_to_copy);
  xQueueOverwrite(led_strip_queue, &frame);
}

SubscriptionEntry led_strip_sub = {
    .msg = &s_img_msg,
    .type_support = ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Image),
    .topic_name = "led_strip",
    .callback = ledStripCallback,
    .best_effort = true,
};

// Motors subscriber
static std_msgs__msg__Float32MultiArray s_mot_msg = {
    .layout = {},
    .data =
        {
            .data = new float[MAX_NUM_MOTORS](),
            .size = 0,
            .capacity = MAX_NUM_MOTORS,
        },
};

void motorsCmdCallback(const void* msg_in) {
  if (msg_in == nullptr) return;
  auto msg = static_cast<const std_msgs__msg__Float32MultiArray*>(msg_in);

  const uint8_t n = g_motors.count();
  if (msg->data.size >= n) {
    g_motors.setVelocities(msg->data.data);
  }
}

SubscriptionEntry motor_sub = {
    .msg = &s_mot_msg,
    .type_support =
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
    .topic_name = "_motors/cmd",
    .callback = motorsCmdCallback,
    .best_effort = true,
};

static std::vector<SubscriptionEntry> subscriptions = {
    leds_sub,
    led_strip_sub,
    motor_sub,
};

// SERVICES
rcl_service_t mcu_id_service;
std_srvs__srv__Trigger_Request mcu_id_req;
std_srvs__srv__Trigger_Response mcu_id_res;

void mcuIdCallback(const void* req, void* res) {
  constexpr uint32_t MCU_UID = 0x1FFF7A10;
  constexpr uint8_t NUM_BYTES = 12;
  uint8_t buffer[NUM_BYTES];
  memcpy(buffer, (void*)MCU_UID, NUM_BYTES);

  // Prepare the CPU ID in hexadecimal format
  char mcu_id_buffer[NUM_BYTES * 2 + 1] = {0};
  char* hex_ptr = mcu_id_buffer;
  for (uint8_t i = 0; i < NUM_BYTES; ++i) {
    snprintf(hex_ptr, 3, "%02X", buffer[i]);
    hex_ptr += 2;
  }

  // Prepare the final output buffer with "CPU ID: " prefix
  static char out_buffer[100];  // Ensure this is large enough
  snprintf(out_buffer, sizeof(out_buffer), "{\"mcu_id\": \"%s\"}",
           mcu_id_buffer);

  // Response
  std_srvs__srv__Trigger_Response* response =
      (std_srvs__srv__Trigger_Response*)res;
  response->success = true;
  response->message.data = out_buffer;
  response->message.size = strlen(out_buffer);
}

static std::vector<ServiceEntry> services = {
    {
        .srv = {},
        .request = &mcu_id_req,
        .response = &mcu_id_res,
        .type_support = ROSIDL_GET_SRV_TYPE_SUPPORT(std_srvs, srv, Trigger),
        .service_name = "_mcu_id",
        .callback = mcuIdCallback,
    },
};

RosNodeConfig ros_node_config = {.node_name = NODE_NAME,
                                 .domain_id = DOMAIN_ID,
                                 .publishers = publishers.data(),
                                 .pub_count = publishers.size(),
                                 .subscriptions = subscriptions.data(),
                                 .sub_count = subscriptions.size(),
                                 .services = services.data(),
                                 .srv_count = services.size(),
                                 .spin_time_ms = SPIN_TIME_MS,
                                 .timer_ms = TIMER_MS,
                                 .ping_watchdog_ms = PING_WATCHDOG_MS,
                                 .ping_attempts = PING_ATTEMPTS,
                                 .ping_timeout_ms = PING_TIMEOUT_MS};

RosNode g_ros_node(ros_node_config);
