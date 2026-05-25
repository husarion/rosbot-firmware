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

#include "rosbot_mavlink_bridge/bridge_node.hpp"

#include <chrono>
#include <cstring>

#include "rclcpp/qos.hpp"

namespace rosbot_mavlink_bridge {

namespace {
constexpr std::uint8_t kChannel = MAVLINK_COMM_0;
constexpr std::size_t kMaxFrame = MAVLINK_MAX_PACKET_LEN;

rclcpp::QoS bestEffortDepth1() {
  return rclcpp::QoS(rclcpp::KeepLast(1)).best_effort();
}
}  // namespace

BridgeNode::BridgeNode(const rclcpp::NodeOptions& node_options,
                       std::unique_ptr<Transport> transport)
    : rclcpp::Node("rosbot_mcu", node_options),
      transport_(std::move(transport)) {
  // ── Parameters ─────────────────────────────────────────
  enable_ranges_ = this->declare_parameter<bool>("enable_ranges", false);
  enable_led_strip_ = this->declare_parameter<bool>("enable_led_strip", false);
  publish_link_state_ =
      this->declare_parameter<bool>("publish_link_state", false);
  timesync_alpha_ = this->declare_parameter<double>("timesync_alpha", 0.05);
  banner_regex_str_ = this->declare_parameter<std::string>(
      "expected_banner_regex", "rosbot(?:_xl)? .* mavlink");
  banner_regex_ = std::regex(banner_regex_str_);
  // Grace window for a bridge that started after the firmware finished
  // emitting its 10-shot banner; promote on HEARTBEAT alone after this
  // many seconds. 0 = strictly require the banner.
  banner_grace_seconds_ =
      this->declare_parameter<int>("banner_grace_seconds", 8);

  if (!transport_->open()) {
    RCLCPP_FATAL(this->get_logger(),
                 "Failed to open MAVLink transport — aborting bridge node.");
    throw std::runtime_error("transport open failed");
  }
  RCLCPP_INFO(this->get_logger(),
              "MAVLink transport open. Waiting for HEARTBEAT + boot banner.");

  // Topics + QoS mirror the micro-ROS API contract.
  battery_pub_ = this->create_publisher<sensor_msgs::msg::BatteryState>(
      "battery", bestEffortDepth1());
  imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>("_imu/data",
                                                           bestEffortDepth1());
  joint_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>(
      "_motors/feedback", bestEffortDepth1());
  buttons_pub_ = this->create_publisher<std_msgs::msg::UInt8>(
      "buttons", bestEffortDepth1());
  if (enable_ranges_) {
    range_pub_ = this->create_publisher<sensor_msgs::msg::Range>(
        "ranges", bestEffortDepth1());
  }
  if (publish_link_state_) {
    link_state_pub_ = this->create_publisher<std_msgs::msg::UInt8>(
        "mcu_link_state", bestEffortDepth1());
  }

  wheel_cmd_sub_ = this->create_subscription<std_msgs::msg::Float32MultiArray>(
      "_motors/cmd", bestEffortDepth1(),
      [this](std_msgs::msg::Float32MultiArray::SharedPtr m) { wheelCmdCb(m); });
  leds_sub_ = this->create_subscription<std_msgs::msg::UInt8>(
      "leds", bestEffortDepth1(),
      [this](std_msgs::msg::UInt8::SharedPtr m) { ledsCb(m); });
  if (enable_led_strip_) {
    led_strip_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
        "led_strip", bestEffortDepth1(),
        [this](sensor_msgs::msg::Image::SharedPtr m) { ledStripCb(m); });
  }
  mcu_id_service_ = this->create_service<std_srvs::srv::Trigger>(
      "_mcu_id",
      [this](const std::shared_ptr<std_srvs::srv::Trigger::Request> req,
             std::shared_ptr<std_srvs::srv::Trigger::Response> res) {
        mcuIdServiceCb(req, res);
      });

  heartbeat_timer_ = this->create_wall_timer(std::chrono::seconds(1),
                                             [this]() { heartbeatTimer(); });

  rx_running_.store(true);
  rx_thread_ = std::thread(&BridgeNode::rxLoop, this);
}

BridgeNode::~BridgeNode() {
  rx_running_.store(false);
  if (rx_thread_.joinable()) rx_thread_.join();
  if (transport_) transport_->close();
}

std::int64_t BridgeNode::nowUnixNs() const { return this->now().nanoseconds(); }

rclcpp::Time BridgeNode::mcuTimeToRos(std::uint64_t time_boot_us) const {
  if (!time_synced_.load()) {
    // Stamps fall back to bridge wall clock until TIMESYNC converges (~1 s).
    return this->now();
  }
  std::int64_t mcu_boot_ns = static_cast<std::int64_t>(time_boot_us) * 1000LL;
  std::int64_t unix_ns = mcu_boot_ns + time_offset_ns_.load();
  return rclcpp::Time(unix_ns);
}

void BridgeNode::sendMavlink(mavlink_message_t& msg) {
  std::uint8_t buf[kMaxFrame];
  std::uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  std::lock_guard<std::mutex> lk(tx_mutex_);
  transport_->write(buf, len);
}

void BridgeNode::heartbeatTimer() {
  mavlink_message_t m;
  mavlink_msg_heartbeat_pack(bridge_sysid_, bridge_compid_, &m,
                             MAV_TYPE_ONBOARD_CONTROLLER, MAV_AUTOPILOT_INVALID,
                             0, 0, MAV_STATE_ACTIVE);
  sendMavlink(m);

  if (publish_link_state_ && link_state_pub_) {
    std_msgs::msg::UInt8 msg;
    // bit0: peer alive, bit1: banner seen, bit2: time synced.
    msg.data = (peer_alive_.load() ? 0x1 : 0x0) |
               (banner_seen_.load() ? 0x2 : 0x0) |
               (time_synced_.load() ? 0x4 : 0x0);
    link_state_pub_->publish(msg);
  }

  const auto now_ns = nowUnixNs();
  const auto last = last_peer_heartbeat_ns_.load();
  if (peer_alive_.load() && last != 0 &&
      (now_ns - last) > std::chrono::nanoseconds(peer_timeout_).count()) {
    peer_alive_.store(false);
    RCLCPP_WARN(this->get_logger(),
                "MCU HEARTBEAT timeout (%ld ms) — declaring DISCONNECTED.",
                peer_timeout_.count());
  }

  if (!banner_seen_.load() && peer_alive_.load() && banner_grace_seconds_ > 0) {
    if (first_peer_heartbeat_ns_ == 0) {
      first_peer_heartbeat_ns_ = last_peer_heartbeat_ns_.load();
    }
    const auto grace_ns =
        std::chrono::nanoseconds(std::chrono::seconds(banner_grace_seconds_))
            .count();
    if ((now_ns - first_peer_heartbeat_ns_) > grace_ns) {
      banner_seen_.store(true);
      RCLCPP_WARN(this->get_logger(),
                  "No boot banner seen within %ds of first HEARTBEAT — "
                  "promoting anyway (firmware likely booted before bridge).",
                  banner_grace_seconds_);
    }
  }

  if (!banner_seen_.load()) {
    static auto last_log = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (now - last_log > std::chrono::seconds(5)) {
      RCLCPP_WARN(this->get_logger(),
                  "Still waiting for firmware boot banner matching '%s'.",
                  banner_regex_str_.c_str());
      last_log = now;
    }
  }
}

void BridgeNode::rxLoop() {
  // Reads a full datagram (or whatever serial has) so UDP doesn't drop the
  // tail of a frame between read() calls; the parser is byte-by-byte.
  std::uint8_t buf[kMaxFrame];
  mavlink_message_t msg;
  mavlink_status_t status;
  std::memset(&status, 0, sizeof(status));

  while (rx_running_.load()) {
    // 50 ms timeout keeps the thread responsive to shutdown while idle.
    const std::size_t n = transport_->read(buf, sizeof(buf), 50);
    for (std::size_t i = 0; i < n; ++i) {
      if (mavlink_parse_char(kChannel, buf[i], &msg, &status)) {
        onMavlinkMessage(msg);
      }
    }
  }
}

void BridgeNode::onMavlinkMessage(const mavlink_message_t& msg) {
  switch (msg.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT:
      onHeartbeat(msg);
      break;
    case MAVLINK_MSG_ID_TIMESYNC:
      onTimesync(msg);
      break;
    case MAVLINK_MSG_ID_STATUSTEXT:
      onStatustext(msg);
      break;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
      onBatteryStatus(msg);
      break;
    case MAVLINK_MSG_ID_ROSBOT_IMU:
      onRosbotImu(msg);
      break;
    case MAVLINK_MSG_ID_ROSBOT_JOINT_STATE:
      onRosbotJointState(msg);
      break;
    case MAVLINK_MSG_ID_ROSBOT_BUTTONS:
      onRosbotButtons(msg);
      break;
    case MAVLINK_MSG_ID_DISTANCE_SENSOR:
      onDistanceSensor(msg);
      break;
    case MAVLINK_MSG_ID_ROSBOT_MCU_ID:
      onRosbotMcuId(msg);
      break;
    case MAVLINK_MSG_ID_COMMAND_ACK:
      onCommandAck(msg);
      break;
    default:
      break;
  }
}

void BridgeNode::onHeartbeat(const mavlink_message_t& msg) {
  if (msg.sysid != mcu_sysid_) return;
  last_peer_heartbeat_ns_.store(nowUnixNs());
  if (!peer_alive_.exchange(true)) {
    RCLCPP_INFO(this->get_logger(), "Saw MCU HEARTBEAT (sysid=%u compid=%u).",
                msg.sysid, msg.compid);
  }
}

void BridgeNode::onTimesync(const mavlink_message_t& msg) {
  mavlink_timesync_t ts;
  mavlink_msg_timesync_decode(&msg, &ts);

  // tc1==0 → MCU-initiated; echo ts1 and update our offset estimate from
  // the same exchange. RTT on UDP is sub-ms, EWMA absorbs the asymmetry.
  if (ts.tc1 == 0) {
    const std::int64_t bridge_unix_ns = nowUnixNs();
    mavlink_message_t reply;
    mavlink_msg_timesync_pack(bridge_sysid_, bridge_compid_, &reply,
                              bridge_unix_ns, ts.ts1);
    sendMavlink(reply);

    const std::int64_t sample = bridge_unix_ns - ts.ts1;
    std::int64_t prev = time_offset_ns_.load();
    std::int64_t next;
    if (!time_synced_.load()) {
      next = sample;
      time_synced_.store(true);
      RCLCPP_INFO(this->get_logger(),
                  "TIMESYNC: initial MCU-boot → wall offset = %ld ns.", sample);
    } else {
      // Cast through double — direct int64 multiply wraps on early boot
      // when prev/sample still diverge widely.
      const double a = timesync_alpha_;
      next = static_cast<std::int64_t>(a * static_cast<double>(sample) +
                                       (1.0 - a) * static_cast<double>(prev));
    }
    time_offset_ns_.store(next);
  }
}

void BridgeNode::onStatustext(const mavlink_message_t& msg) {
  mavlink_statustext_t st;
  mavlink_msg_statustext_decode(&msg, &st);
  // STATUSTEXT.text may not be NUL-terminated; trailing byte forces one.
  char text[51];
  std::memcpy(text, st.text, 50);
  text[50] = '\0';

  if (st.severity <= MAV_SEVERITY_ERROR) {
    RCLCPP_ERROR(this->get_logger(), "[MCU] %s", text);
  } else if (st.severity <= MAV_SEVERITY_WARNING) {
    RCLCPP_WARN(this->get_logger(), "[MCU] %s", text);
  } else if (st.severity <= MAV_SEVERITY_INFO) {
    RCLCPP_INFO(this->get_logger(), "[MCU] %s", text);
  } else {
    RCLCPP_DEBUG(this->get_logger(), "[MCU] %s", text);
  }

  if (!banner_seen_.load() &&
      std::regex_search(text, text + std::strlen(text), banner_regex_)) {
    banner_seen_.store(true);
    RCLCPP_INFO(this->get_logger(),
                "Boot banner matched ('%s') — bridge CONNECTED.", text);
  }
}

void BridgeNode::onBatteryStatus(const mavlink_message_t& msg) {
  if (!peer_alive_.load() || !banner_seen_.load()) return;

  mavlink_battery_status_t bs;
  mavlink_msg_battery_status_decode(&msg, &bs);

  sensor_msgs::msg::BatteryState out;
  out.header.stamp = this->now();
  out.header.frame_id = "base_link";
  // Voltages are per-cell mV; sum the valid slots for the pack total.
  float total_v = 0.0f;
  out.cell_voltage.clear();
  for (int i = 0; i < 10; ++i) {
    if (bs.voltages[i] == UINT16_MAX) break;
    float v = static_cast<float>(bs.voltages[i]) * 1e-3f;
    out.cell_voltage.push_back(v);
    total_v += v;
  }
  out.voltage = total_v;
  out.current = (bs.current_battery == -1)
                    ? std::numeric_limits<float>::quiet_NaN()
                    : static_cast<float>(bs.current_battery) * 1e-2f;
  out.percentage = (bs.battery_remaining < 0)
                       ? std::numeric_limits<float>::quiet_NaN()
                       : static_cast<float>(bs.battery_remaining) * 1e-2f;
  out.temperature = std::numeric_limits<float>::quiet_NaN();
  out.charge = std::numeric_limits<float>::quiet_NaN();
  out.capacity = std::numeric_limits<float>::quiet_NaN();
  out.design_capacity = 7.8f;  // matches micro-ROS BATTERY_DESIGN_CAPACITY
  out.power_supply_status =
      sensor_msgs::msg::BatteryState::POWER_SUPPLY_STATUS_UNKNOWN;
  out.power_supply_health =
      sensor_msgs::msg::BatteryState::POWER_SUPPLY_HEALTH_UNKNOWN;
  out.power_supply_technology =
      sensor_msgs::msg::BatteryState::POWER_SUPPLY_TECHNOLOGY_LION;
  out.present = true;
  out.location = "internal";
  out.serial_number = "";
  battery_pub_->publish(out);
}

void BridgeNode::onRosbotImu(const mavlink_message_t& msg) {
  if (!peer_alive_.load() || !banner_seen_.load()) return;

  mavlink_rosbot_imu_t i;
  mavlink_msg_rosbot_imu_decode(&msg, &i);

  sensor_msgs::msg::Imu out;
  out.header.stamp = mcuTimeToRos(i.time_boot_us);
  out.header.frame_id = "imu_link";
  out.orientation.x = i.quaternion[0];
  out.orientation.y = i.quaternion[1];
  out.orientation.z = i.quaternion[2];
  out.orientation.w = i.quaternion[3];
  out.angular_velocity.x = i.angular_velocity[0];
  out.angular_velocity.y = i.angular_velocity[1];
  out.angular_velocity.z = i.angular_velocity[2];
  out.linear_acceleration.x = i.linear_acceleration[0];
  out.linear_acceleration.y = i.linear_acceleration[1];
  out.linear_acceleration.z = i.linear_acceleration[2];
  // ROS convention: covariance[0] = -1 marks the whole 3x3 as unknown.
  for (auto* arr :
       {&out.orientation_covariance, &out.angular_velocity_covariance,
        &out.linear_acceleration_covariance}) {
    arr->fill(0.0);
    (*arr)[0] = -1.0;
  }
  imu_pub_->publish(out);
}

void BridgeNode::onRosbotJointState(const mavlink_message_t& msg) {
  if (!peer_alive_.load() || !banner_seen_.load()) return;

  mavlink_rosbot_joint_state_t js;
  mavlink_msg_rosbot_joint_state_decode(&msg, &js);

  sensor_msgs::msg::JointState out;
  out.header.stamp = mcuTimeToRos(js.time_boot_us);
  out.header.frame_id = "base_link";
  out.name.assign(joint_names_.begin(), joint_names_.end());
  out.position.assign(js.position, js.position + 4);
  out.velocity.assign(js.velocity, js.velocity + 4);
  out.effort.assign(js.effort, js.effort + 4);
  joint_state_pub_->publish(out);
}

void BridgeNode::onRosbotButtons(const mavlink_message_t& msg) {
  if (!peer_alive_.load() || !banner_seen_.load()) return;

  mavlink_rosbot_buttons_t b;
  mavlink_msg_rosbot_buttons_decode(&msg, &b);
  std_msgs::msg::UInt8 out;
  out.data = b.mask;
  buttons_pub_->publish(out);
}

void BridgeNode::onDistanceSensor(const mavlink_message_t& msg) {
  if (!enable_ranges_ || !range_pub_) return;
  if (!peer_alive_.load() || !banner_seen_.load()) return;

  mavlink_distance_sensor_t d;
  mavlink_msg_distance_sensor_decode(&msg, &d);

  sensor_msgs::msg::Range out;
  out.header.stamp =
      mcuTimeToRos(static_cast<std::uint64_t>(d.time_boot_ms) * 1000ULL);
  if (d.id < range_frames_.size()) {
    out.header.frame_id = range_frames_[d.id];
  }
  out.radiation_type = sensor_msgs::msg::Range::INFRARED;
  out.field_of_view = 0.26f;
  out.min_range = static_cast<float>(d.min_distance) * 1e-2f;
  out.max_range = static_cast<float>(d.max_distance) * 1e-2f;
  const float range_m = static_cast<float>(d.current_distance) * 1e-2f;
  if (range_m > out.max_range) {
    out.range = std::numeric_limits<float>::infinity();
  } else if (range_m < out.min_range) {
    out.range = -std::numeric_limits<float>::infinity();
  } else {
    out.range = range_m;
  }
  range_pub_->publish(out);
}

void BridgeNode::onRosbotMcuId(const mavlink_message_t& msg) {
  mavlink_rosbot_mcu_id_t m;
  mavlink_msg_rosbot_mcu_id_decode(&msg, &m);
  char buf[25];
  std::memcpy(buf, m.uid, 24);
  buf[24] = '\0';

  {
    std::lock_guard<std::mutex> lk(mcu_id_mutex_);
    mcu_id_value_ = buf;
    mcu_id_received_ = true;
  }
  mcu_id_cv_.notify_all();
}

void BridgeNode::onCommandAck(const mavlink_message_t& msg) {
  // We retry COMMAND_LONG on timeout rather than tracking ACKs.
  (void)msg;
}

void BridgeNode::wheelCmdCb(
    const std_msgs::msg::Float32MultiArray::SharedPtr msg) {
  if (msg->data.size() < 4) return;
  mavlink_message_t m;
  mavlink_msg_rosbot_wheel_setpoints_pack(
      bridge_sysid_, bridge_compid_, &m,
      static_cast<std::uint64_t>(nowUnixNs() / 1000), msg->data.data());
  sendMavlink(m);
}

void BridgeNode::ledsCb(const std_msgs::msg::UInt8::SharedPtr msg) {
  mavlink_message_t m;
  mavlink_msg_rosbot_panel_leds_pack(bridge_sysid_, bridge_compid_, &m,
                                     msg->data);
  sendMavlink(m);
}

void BridgeNode::ledStripCb(const sensor_msgs::msg::Image::SharedPtr msg) {
  if (msg->height != 1) return;
  if (msg->encoding != "rgb8") return;
  if (msg->width == 0 || msg->width > 18) return;

  std::uint8_t rgb[54] = {0};
  const std::size_t valid_bytes = std::min<std::size_t>(msg->data.size(), 54);
  std::memcpy(rgb, msg->data.data(), valid_bytes);

  mavlink_message_t m;
  mavlink_msg_rosbot_led_strip_pack(bridge_sysid_, bridge_compid_, &m,
                                    static_cast<std::uint8_t>(msg->width), rgb);
  sendMavlink(m);
}

void BridgeNode::mcuIdServiceCb(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> /*req*/,
    std::shared_ptr<std_srvs::srv::Trigger::Response> res) {
  // COMMAND_LONG(MAV_CMD_USER_1) → ROSBOT_MCU_ID; retry on no reply.
  {
    std::lock_guard<std::mutex> lk(mcu_id_mutex_);
    mcu_id_received_ = false;
    mcu_id_pending_ = true;
  }

  for (int attempt = 0; attempt < 3; ++attempt) {
    mavlink_message_t cmd;
    mavlink_msg_command_long_pack(bridge_sysid_, bridge_compid_, &cmd,
                                  mcu_sysid_, mcu_compid_, MAV_CMD_USER_1,
                                  /*confirmation=*/attempt, 0, 0, 0, 0, 0, 0,
                                  0);
    sendMavlink(cmd);

    std::unique_lock<std::mutex> lk(mcu_id_mutex_);
    if (mcu_id_cv_.wait_for(lk, std::chrono::milliseconds(500),
                            [this] { return mcu_id_received_; })) {
      res->success = true;
      res->message = std::string("{\"mcu_id\": \"") + mcu_id_value_ + "\"}";
      mcu_id_pending_ = false;
      return;
    }
  }

  res->success = false;
  res->message = "{\"mcu_id\": \"\"}";
  std::lock_guard<std::mutex> lk(mcu_id_mutex_);
  mcu_id_pending_ = false;
}

}  // namespace rosbot_mavlink_bridge
