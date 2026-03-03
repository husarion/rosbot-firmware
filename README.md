# rosbot-firmware

Unified micro-ros based firmware for ROSbot 3 and ROSbot XL for STM32F4 microcontroller.

## VS Code Tasks

To simplify the development process, we have prepared a set of VS Code tasks that can be used to build and flash the firmware.

To use these tasks, open the Command Palette (Ctrl+Shift+P) and search for "Run Task". You will see a list of available tasks, including:

- **ROSbot: Build firmware**
- **ROSbot: Build and deploy firmware**
- **ROSbot XL: Build firmware**
- **ROSbot XL: Build and deploy firmware**
- **Build Releases**

## Usage

The firmware consists of three phases:
- connection selection phase
- pre-communication phase
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
