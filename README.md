# rosbot-firmware

Unified micro-ros based firmware for ROSbot 3 and ROSbot XL for STM32F4 microcontroller.

## Usage

The firmware consists of three phases:
- connection selection phase [(dev mode)](./CONTRIBUTING.md#dev-mode)
- pre-communication phase (namespace configuration)
- main phase - communication via micro-ROS

To make a connection build micro ROS agent and connect to it using:

Ethernet:
```bash
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```

Serial:
```bash
ros2 run micro_ros_agent micro_ros_agent serial --dev <serial_port> --baud <baud_rate>
```
