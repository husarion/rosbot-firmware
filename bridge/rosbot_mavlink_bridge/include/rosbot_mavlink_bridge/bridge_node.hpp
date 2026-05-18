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

#include "mavlink.h"
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

namespace rosbot_mavlink_bridge {

class BridgeNode : public rclcpp::Node {
 public:
  /// @param node_options the namespace etc. is set by the caller before
  /// constructing the node (see main.cpp).
  /// @param transport already-constructed transport (UDP or serial).
  BridgeNode(const rclcpp::NodeOptions& node_options,
             std::unique_ptr<Transport> transport);
  ~BridgeNode() override;

 private:
  // ── Internal threads / loops ─────────────────────────────
  void rxLoop();
  void heartbeatTimer();
  void onMavlinkMessage(const mavlink_message_t& msg);

  // ── Wire helpers ─────────────────────────────────────────
  void sendMavlink(mavlink_message_t& msg);
  rclcpp::Time mcuTimeToRos(std::uint64_t time_boot_us) const;
  std::int64_t nowUnixNs() const;

  // ── Telemetry handlers (MAVLink → ROS) ───────────────────
  void onHeartbeat(const mavlink_message_t& msg);
  void onTimesync(const mavlink_message_t& msg);
  void onStatustext(const mavlink_message_t& msg);
  void onBatteryStatus(const mavlink_message_t& msg);
  void onRosbotImu(const mavlink_message_t& msg);
  void onRosbotJointState(const mavlink_message_t& msg);
  void onRosbotButtons(const mavlink_message_t& msg);
  void onDistanceSensor(const mavlink_message_t& msg);
  void onRosbotMcuId(const mavlink_message_t& msg);
  void onCommandAck(const mavlink_message_t& msg);

  // ── Command callbacks (ROS → MAVLink) ────────────────────
  void wheelCmdCb(const std_msgs::msg::Float32MultiArray::SharedPtr msg);
  void ledsCb(const std_msgs::msg::UInt8::SharedPtr msg);
  void ledStripCb(const sensor_msgs::msg::Image::SharedPtr msg);
  void mcuIdServiceCb(
      const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
      std::shared_ptr<std_srvs::srv::Trigger::Response> res);

  // ── Member state ─────────────────────────────────────────
  std::unique_ptr<Transport> transport_;
  std::thread rx_thread_;
  std::atomic<bool> rx_running_{false};
  std::mutex tx_mutex_;

  // Identity (D6): MCU is sysid=1/compid=AUTOPILOT1. Bridge uses 255/USER1
  // — outside the MAVLink autopilot range so we don't impersonate a peer.
  std::uint8_t bridge_sysid_ = 255;
  std::uint8_t bridge_compid_ = MAV_COMP_ID_USER1;
  std::uint8_t mcu_sysid_ = 1;
  std::uint8_t mcu_compid_ = MAV_COMP_ID_AUTOPILOT1;

  // Time sync — offset = bridge_unix_ns - mcu_boot_ns (filtered EWMA).
  // Applied to every published telemetry stamp (§10.2 step 4).
  std::atomic<std::int64_t> time_offset_ns_{0};
  std::atomic<bool> time_synced_{false};
  double timesync_alpha_ = 0.05;

  // Connection state. We require BOTH a peer HEARTBEAT in the last 3 s
  // AND the boot banner before we publish telemetry. Banner parsing is the
  // D19 mismatch-detection gate.
  std::atomic<bool> peer_alive_{false};
  std::atomic<bool> banner_seen_{false};
  std::atomic<std::int64_t> last_peer_heartbeat_ns_{0};
  std::chrono::milliseconds peer_timeout_{3000};
  std::regex banner_regex_;
  std::string banner_regex_str_;
  int banner_grace_seconds_ = 0;
  std::int64_t first_peer_heartbeat_ns_ = 0;

  // MCU ID service correlation: bridge sends COMMAND_LONG, MCU returns
  // ROSBOT_MCU_ID. We hand the latest payload to whichever Trigger client
  // is waiting (only one at a time supported — the public service is
  // synchronous, so concurrent calls would queue at the rclcpp executor).
  std::mutex mcu_id_mutex_;
  std::condition_variable mcu_id_cv_;
  std::string mcu_id_value_;
  bool mcu_id_pending_ = false;
  bool mcu_id_received_ = false;

  // ROS-side interfaces. All names are relative — namespace prefix comes
  // from rclcpp via the launch-arg pipeline (§10.1).
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

  // Variant flags driven by parameters. Rosbot has 4 ToF ranges; rosbot_xl
  // has an LED strip subscriber. Sharing the bridge class avoids two
  // near-identical executables.
  bool enable_ranges_ = false;
  bool enable_led_strip_ = false;

  // Bridge-internal diagnostics topic (§10.2 step 8) — off by default.
  bool publish_link_state_ = false;
  rclcpp::Publisher<std_msgs::msg::UInt8>::SharedPtr link_state_pub_;

  // Constant tables — wheel order matches the dialect contract (D9) and
  // range id → frame mapping (§4.2).
  std::array<std::string, 4> joint_names_{"fl_wheel_joint", "fr_wheel_joint",
                                          "rl_wheel_joint", "rr_wheel_joint"};
  std::array<std::string, 4> range_frames_{"fl_range", "fr_range", "rl_range",
                                           "rr_range"};
};

}  // namespace rosbot_mavlink_bridge
