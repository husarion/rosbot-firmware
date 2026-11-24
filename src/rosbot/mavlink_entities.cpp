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

#include <cmath>

#include "commands/imu_calibration_command.hpp"
#include "commands/mcu_id_command.hpp"
#include "mavlink_node.hpp"
#include "publishers/battery_publisher.hpp"
#include "publishers/buttons_publisher.hpp"
#include "publishers/diagnostics_publisher.hpp"
#include "publishers/imu_calibration_publisher.hpp"
#include "publishers/imu_publisher.hpp"
#include "publishers/joint_state_publisher.hpp"
#include "publishers/range_publisher.hpp"
#include "rosbot/config.hpp"
#include "subscribers/led_subscriber.hpp"
#include "subscribers/wheel_cmd_subscriber.hpp"
#include "transport/mavlink_serial_transport.hpp"

static MavlinkSerialTransport serial_transport(SBC_SERIAL_CONFIG);

// Publishers (telemetry: MCU → bridge)
static MavlinkBatteryPublisher battery_pub(
    {.queue = battery_queue,
     .num_cells = BATTERY_NUM_CELLS,
     .period_ms = 1000,
     .power_supply_design_capacity_mah =
         static_cast<uint16_t>(lroundf(BATTERY_DESIGN_CAPACITY * 1000.0f))});
// IMU, joint state and ranges: period_ms = 0, the producing task sets the
// rate. A gate equal to the task period dropped samples: with a 5 ms link
// loop, ~18% of IMU samples were overwritten in their depth-1 queue before
// the gate reopened (HW-measured 82 Hz on ROSbot 3).
static MavlinkImuPublisher imu_pub({.queue = imu_queue, .period_ms = 0});
// 5 Hz: enough for a progress bar, cheap on the link.
static MavlinkImuCalibrationPublisher imu_calibration_pub({.period_ms = 200});
static MavlinkJointStatePublisher joint_state_pub({.queue = joint_state_queue,
                                                   .period_ms = 0});
static MavlinkRangePublisher range_pub({.queue = ranges_queue,
                                        .min_range_m = 0.01f,
                                        .max_range_m = 0.9f,
                                        .period_ms = 0});
static MavlinkButtonsPublisher buttons_pub({.pins = buttons_pins,
                                            .period_ms = 50});

extern RangeVl53l0x range_fl, range_fr, range_rl, range_rr;

static uint16_t sat16(uint32_t v) {
  return v > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(v);
}

static void fillDiagnostics(mavlink_rosbot_diagnostics_t& d) {
  d.imu_errors = g_imu->readErrors();
  d.imu_ok = g_imu->healthy() ? 1 : 0;
  const RangeVl53l0x* ranges[] = {&range_fl, &range_fr, &range_rl, &range_rr};
  for (size_t i = 0; i < 4; ++i) {
    d.range_timeouts[i] = sat16(ranges[i]->timeouts());
    if (ranges[i]->initialized()) d.range_ok |= 1U << i;
  }
  d.i2c_recoveries[0] = sat16(imu_i2c.recoveries());
  d.i2c_recoveries[1] = sat16(range_i2c.recoveries());
  d.motor_faults = sat16(g_motors.faultEvents());
  d.flags = ROSBOT_DIAG_HAS_RANGES;
  if (g_battery->lowLatched()) d.flags |= ROSBOT_DIAG_BATTERY_LOW;
  if (g_battery->criticalLatched()) d.flags |= ROSBOT_DIAG_BATTERY_CRITICAL;
  if (g_motors.hasFault()) d.flags |= ROSBOT_DIAG_MOTOR_FAULT;
}

static MavlinkDiagnosticsPublisher diagnostics_pub({.fill = fillDiagnostics,
                                                    .period_ms = 1000});

static MavlinkPublisherInterface* s_publishers[] = {
    &battery_pub, &imu_pub,     &imu_calibration_pub, &joint_state_pub,
    &range_pub,   &buttons_pub, &diagnostics_pub};

// Subscribers (commands: bridge → MCU)
static const PanelLedConfig s_panel_leds[] = {
    {.pin = GRN_LED, .bit_mask = 0x01},
    {.pin = GRN_LED2, .bit_mask = 0x02},
};
static WheelCmdSubscriber wheel_cmd_sub(g_motors);
static PanelLedSubscriber leds_sub(s_panel_leds);
static McuIdCommand mcu_id_cmd;
static ImuCalibrationCommand imu_calibration_cmd;

static MavlinkSubscriberInterface* s_subscribers[] = {
    &wheel_cmd_sub, &leds_sub, &mcu_id_cmd, &imu_calibration_cmd};

static MavlinkNodeConfig mavlink_cfg = {
    .sysid = 1,
    .compid = MAV_COMP_ID_AUTOPILOT1,
    .mav_type = MAV_TYPE_GROUND_ROVER,
    .autopilot = MAV_AUTOPILOT_GENERIC,
    .boot_banner = "rosbot " FW_VERSION " mavlink",
    .heartbeat_period_ms = 1000,
    .timesync_period_ms = 2000,
    .peer_timeout_ms = 3000,
    .publishers = s_publishers,
    .pub_count = sizeof(s_publishers) / sizeof(s_publishers[0]),
    .subscribers = s_subscribers,
    .sub_count = sizeof(s_subscribers) / sizeof(s_subscribers[0]),
};

MavlinkNode g_mavlink_node(serial_transport, mavlink_cfg);
