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

#include "commands/mcu_id_command.hpp"
#include "config.hpp"
#include "mavlink_node.hpp"
#include "publishers/battery_publisher.hpp"
#include "publishers/buttons_publisher.hpp"
#include "publishers/imu_publisher.hpp"
#include "publishers/joint_state_publisher.hpp"
#include "publishers/range_publisher.hpp"
#include "subscribers/led_subscriber.hpp"
#include "subscribers/wheel_cmd_subscriber.hpp"
#include "transport/mavlink_serial_transport.hpp"

// Same SBC serial pinout/baud as the micro-ROS path; only framing differs.
static MavlinkSerialTransport serial_transport(SBC_SERIAL_CONFIG);

// Publishers (telemetry: MCU → bridge)
static MavlinkBatteryPublisher battery_pub({.queue = battery_queue,
                                            .num_cells = BATTERY_NUM_CELLS,
                                            .period_ms = 1000});
static MavlinkImuPublisher imu_pub({.queue = imu_queue, .period_ms = 10});
static MavlinkJointStatePublisher joint_state_pub({.queue = joint_state_queue,
                                                   .period_ms = 5});
static MavlinkRangePublisher range_pub({.queue = ranges_queue,
                                        .min_range_m = 0.01f,
                                        .max_range_m = 0.9f,
                                        .period_ms = 100});
static MavlinkButtonsPublisher buttons_pub({.pins = buttons_pins,
                                            .num_buttons = sizeof(buttons_pins),
                                            .period_ms = 50});

static MavlinkPublisherInterface* s_publishers[] = {
    &battery_pub, &imu_pub, &joint_state_pub, &range_pub, &buttons_pub};

// Subscribers (commands: bridge → MCU)
static const PanelLedConfig s_panel_leds[] = {
    {.pin = GRN_LED, .bit_mask = 0x01},
    {.pin = GRN_LED2, .bit_mask = 0x02},
};
static WheelCmdSubscriber wheel_cmd_sub(g_motors);
static PanelLedSubscriber leds_sub(s_panel_leds, sizeof(s_panel_leds) /
                                                     sizeof(s_panel_leds[0]));
static McuIdCommand mcu_id_cmd;

static MavlinkSubscriberInterface* s_subscribers[] = {&wheel_cmd_sub, &leds_sub,
                                                      &mcu_id_cmd};

static MavlinkNodeConfig mavlink_cfg = {
    .sysid = 1,
    .compid = MAV_COMP_ID_AUTOPILOT1,
    .mav_type = MAV_TYPE_GROUND_ROVER,
    .autopilot = MAV_AUTOPILOT_GENERIC,
    .boot_banner = "rosbot " FW_VERSION " mavlink",
    .heartbeat_period_ms = 1000,
    .timesync_period_ms = 2000,
    .timesync_active_period_ms = 200,
    .peer_timeout_ms = 3000,
    .publishers = s_publishers,
    .pub_count = sizeof(s_publishers) / sizeof(s_publishers[0]),
    .subscribers = s_subscribers,
    .sub_count = sizeof(s_subscribers) / sizeof(s_subscribers[0]),
};

MavlinkNode g_mavlink_node(serial_transport, mavlink_cfg);
