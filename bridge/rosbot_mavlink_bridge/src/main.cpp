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

#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rosbot_mavlink_bridge/bridge_node.hpp"
#include "rosbot_mavlink_bridge/transport/serial_transport.hpp"
#include "rosbot_mavlink_bridge/transport/udp_transport.hpp"

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);

  // Transport params come off a temporary node so they can be applied
  // before BridgeNode's constructor opens the transport.
  auto bootstrap = std::make_shared<rclcpp::Node>("rosbot_mavlink_bridge_boot");
  const std::string transport =
      bootstrap->declare_parameter<std::string>("transport", "udp");
  const std::string peer_ip =
      bootstrap->declare_parameter<std::string>("peer_ip", "192.168.77.3");
  const int peer_port = bootstrap->declare_parameter<int>("peer_port", 14555);
  const int local_port = bootstrap->declare_parameter<int>("local_port", 14550);
  const std::string serial_port =
      bootstrap->declare_parameter<std::string>("serial_port", "/dev/ttyUSB0");
  const int serial_baudrate =
      bootstrap->declare_parameter<int>("serial_baudrate", 921600);
  const std::string ros_namespace =
      bootstrap->declare_parameter<std::string>("ros_namespace", "");

  rclcpp::NodeOptions opts;
  std::vector<std::string> args = {"--ros-args"};
  if (!ros_namespace.empty()) {
    args.push_back("-r");
    args.push_back("__ns:=/" + ros_namespace);
  }
  opts.arguments(args);

  std::unique_ptr<rosbot_mavlink_bridge::Transport> tp;
  if (transport == "udp") {
    rosbot_mavlink_bridge::UdpConfig cfg{
        peer_ip,
        static_cast<std::uint16_t>(peer_port),
        static_cast<std::uint16_t>(local_port),
    };
    tp = std::make_unique<rosbot_mavlink_bridge::UdpTransport>(cfg);
  } else if (transport == "serial") {
    rosbot_mavlink_bridge::SerialConfig cfg{serial_port, serial_baudrate};
    tp = std::make_unique<rosbot_mavlink_bridge::SerialTransport>(cfg);
  } else {
    RCLCPP_FATAL(bootstrap->get_logger(),
                 "Unknown transport '%s'. Use 'udp' or 'serial'.",
                 transport.c_str());
    rclcpp::shutdown();
    return 1;
  }

  bootstrap.reset();
  auto node =
      std::make_shared<rosbot_mavlink_bridge::BridgeNode>(opts, std::move(tp));
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
