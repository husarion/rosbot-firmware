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
#include "subscribers/led_strip_subscriber.hpp"
#include "subscribers/led_subscriber.hpp"
#include "subscribers/wheel_cmd_subscriber.hpp"
#include "transport/mavlink_udp_transport.hpp"

// MAVROS-default UDP port layout: MCU bound on 14555, peer on 14550.
static MavlinkUdpConfig udp_cfg = {
    .peer_ip = AGENT_IP,
    .peer_port = 14550,
    .local_port = 14555,
};
static MavlinkUdpTransport udp_transport(udp_cfg);

// Publishers (telemetry: MCU → bridge)
static MavlinkBatteryPublisher battery_pub({.queue = battery_queue,
                                            .num_cells = BATTERY_NUM_CELLS,
                                            .period_ms = 1000});
static MavlinkImuPublisher imu_pub({.queue = imu_queue, .period_ms = 10});
static MavlinkJointStatePublisher joint_state_pub({.queue = joint_state_queue,
                                                   .period_ms = 5});
static MavlinkButtonsPublisher buttons_pub({.pins = buttons_pins,
                                            .num_buttons = sizeof(buttons_pins),
                                            .period_ms = 50});

static MavlinkPublisherInterface* s_publishers[] = {
    &battery_pub, &imu_pub, &joint_state_pub, &buttons_pub};

// Subscribers (commands: bridge → MCU)
static const PanelLedConfig s_panel_leds[] = {
    {.pin = GRN_LED, .bit_mask = 0x01},
};
static WheelCmdSubscriber wheel_cmd_sub(g_motors);
static PanelLedSubscriber leds_sub(s_panel_leds, sizeof(s_panel_leds) /
                                                     sizeof(s_panel_leds[0]));
static LedStripSubscriber led_strip_sub(led_strip_queue);
static McuIdCommand mcu_id_cmd;

static MavlinkSubscriberInterface* s_subscribers[] = {
    &wheel_cmd_sub, &leds_sub, &led_strip_sub, &mcu_id_cmd};

static MavlinkNodeConfig mavlink_cfg = {
    .sysid = 1,
    .compid = MAV_COMP_ID_AUTOPILOT1,
    .mav_type = MAV_TYPE_GROUND_ROVER,
    .autopilot = MAV_AUTOPILOT_GENERIC,
    .boot_banner = "rosbot_xl " FW_VERSION " mavlink",
    .heartbeat_period_ms = 1000,
    .timesync_period_ms = 2000,
    .timesync_active_period_ms = 200,
    .peer_timeout_ms = 3000,
    .publishers = s_publishers,
    .pub_count = sizeof(s_publishers) / sizeof(s_publishers[0]),
    .subscribers = s_subscribers,
    .sub_count = sizeof(s_subscribers) / sizeof(s_subscribers[0]),
};

MavlinkNode g_mavlink_node(udp_transport, mavlink_cfg);
