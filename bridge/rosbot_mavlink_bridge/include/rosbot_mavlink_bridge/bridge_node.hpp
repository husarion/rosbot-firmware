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

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <thread>

#include "mavlink.h"  // NOLINT(build/include_subdir) -- mavgen's flat vendored layout
#include "rclcpp/rclcpp.hpp"
#include "rosbot_mavlink_bridge/transport/transport_interface.hpp"
#include "sensor_msgs/msg/battery_state.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "sensor_msgs/msg/range.hpp"
#include "std_msgs/msg/float32_multi_array.hpp"
#include "std_msgs/msg/u_int8.hpp"
#include "std_srvs/srv/trigger.hpp"

namespace rosbot_mavlink_bridge
{

class BridgeNode : public rclcpp::Node {
public:
  BridgeNode(
    const rclcpp::NodeOptions & node_options,
    std::unique_ptr<Transport> transport);
  ~BridgeNode() override;

private:
  void rxLoop();
  void heartbeatTimer();
  void onMavlinkMessage(const mavlink_message_t & msg);

  void sendMavlink(mavlink_message_t & msg);
  rclcpp::Time mcuTimeToRos(std::uint64_t time_boot_us) const;
  std::int64_t nowUnixNs() const;

  // MAVLink → ROS handlers
  void onHeartbeat(const mavlink_message_t & msg);
  void onTimesync(const mavlink_message_t & msg);
  void onStatustext(const mavlink_message_t & msg);
  void onBatteryStatus(const mavlink_message_t & msg);
  void onRosbotImu(const mavlink_message_t & msg);
  void onRosbotJointState(const mavlink_message_t & msg);
  void onRosbotButtons(const mavlink_message_t & msg);
  void onDistanceSensor(const mavlink_message_t & msg);
  void onRosbotMcuId(const mavlink_message_t & msg);
  void onCommandAck(const mavlink_message_t & msg);

  // ROS → MAVLink callbacks
  void wheelCmdCb(const std_msgs::msg::Float32MultiArray::SharedPtr msg);
  void ledsCb(const std_msgs::msg::UInt8::SharedPtr msg);
  void ledStripCb(const sensor_msgs::msg::Image::SharedPtr msg);
  void mcuIdServiceCb(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
    std::shared_ptr<std_srvs::srv::Trigger::Response> res);

  std::unique_ptr<Transport> transport_;
  std::thread rx_thread_;
  std::atomic<bool> rx_running_{false};
  std::mutex tx_mutex_;

  // Bridge identifies as 255/USER1 — outside the autopilot range so it
  // never impersonates the MCU peer (1/AUTOPILOT1).
  std::uint8_t bridge_sysid_ = 255;
  std::uint8_t bridge_compid_ = MAV_COMP_ID_USER1;
  std::uint8_t mcu_sysid_ = 1;
  std::uint8_t mcu_compid_ = MAV_COMP_ID_AUTOPILOT1;

  // EWMA offset = bridge_unix_ns - mcu_boot_ns; applied to telemetry stamps.
  std::atomic<std::int64_t> time_offset_ns_{0};
  std::atomic<bool> time_synced_{false};
  double timesync_alpha_ = 0.05;

  // A recent HEARTBEAT is the sole gate for publishing telemetry. The boot
  // banner is informational — banner_seen_ only latches the one-shot
  // "banner matched" log; the firmware variant is already verified by
  // pre_communication before this node starts.
  std::atomic<bool> peer_alive_{false};
  std::atomic<bool> banner_seen_{false};
  std::atomic<std::int64_t> last_peer_heartbeat_ns_{0};
  std::chrono::milliseconds peer_timeout_{3000};
  std::regex banner_regex_;
  std::string banner_regex_str_;

  // Only one in-flight COMMAND_LONG/ROSBOT_MCU_ID exchange is supported;
  // concurrent service calls queue on the rclcpp executor.
  std::mutex mcu_id_mutex_;
  std::condition_variable mcu_id_cv_;
  std::string mcu_id_value_;
  bool mcu_id_pending_ = false;
  bool mcu_id_received_ = false;

  rclcpp::TimerBase::SharedPtr heartbeat_timer_;
  rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr battery_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
  rclcpp::Publisher<std_msgs::msg::UInt8>::SharedPtr buttons_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Range>::SharedPtr range_pub_;
  rclcpp::Subscription<std_msgs::msg::Float32MultiArray>::SharedPtr
    wheel_cmd_sub_;
  rclcpp::Subscription<std_msgs::msg::UInt8>::SharedPtr leds_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr led_strip_sub_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr mcu_id_service_;

  // rosbot vs rosbot_xl deltas — keeps one executable for both variants.
  bool enable_ranges_ = false;
  bool enable_led_strip_ = false;

  bool publish_link_state_ = false;
  rclcpp::Publisher<std_msgs::msg::UInt8>::SharedPtr link_state_pub_;

  // Wheel order FL/FR/RL/RR matches the dialect; range id → frame_id map.
  std::array<std::string, 4> joint_names_{"fl_wheel_joint", "fr_wheel_joint",
    "rl_wheel_joint", "rr_wheel_joint"};
  std::array<std::string, 4> range_frames_{"fl_range", "fr_range", "rl_range",
    "rr_range"};
};

}  // namespace rosbot_mavlink_bridge
