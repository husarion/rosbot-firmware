# ROS API

This contract is **identical for both firmware flavours** — micro-ROS and
MAVLink. Downstream nodes (e.g. `rosbot_ros`) consume the same node name,
topic list, types, namespacing and QoS regardless of which firmware is
flashed. The bridge (or agent) on the SBC absorbs the wire-protocol
difference; see [README.md](README.md) and [ARCHITECTURE.md](ARCHITECTURE.md)
for which process advertises the API in each case.

## Nodes

[micro_ros_agent/micro_ros_agent]: https://github.com/micro-ROS/micro-ROS-Agent
[rosbot_mavlink_bridge]: ./bridge/rosbot_mavlink_bridge

| NODE             | DESCRIPTION                                                                                                                                                                                       |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`rosbot_mcu`** | Node exposing the ROSbot MCU's topics and services. Advertised by [micro_ros_agent/micro_ros_agent] against the micro-ROS firmware, or by [rosbot_mavlink_bridge] against the MAVLink firmware. |

## Topics

[sensor_msgs/BatteryState]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/BatteryState.html
[sensor_msgs/Image]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/Image.html
[sensor_msgs/Imu]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/Imu.html
[sensor_msgs/JointState]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/JointState.html
[sensor_msgs/Range]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/Range.html
[std_msgs/Float32MultiArray]: https://docs.ros.org/en/jazzy/p/std_msgs/msg/Float32MultiArray.html
[std_msgs/UInt8]: https://docs.ros.org/en/jazzy/p/std_msgs/msg/UInt8.html

| Rb  | Rb XL | TOPIC                  | DESCRIPTION                                                 |
| --- | ----- | ---------------------- | ----------------------------------------------------------- |
| ✅  | ✅    | **`battery`**          | Battery status. <br /> _[sensor_msgs/BatteryState]_         |
| ✅  | ✅    | **`buttons`**          | Button states. <br /> _[std_msgs/UInt8]_                    |
| ❌  | ✅    | **`led_strip`**        | LED strip command. <br /> _[sensor_msgs/Image]_             |
| ✅  | ✅    | **`leds`**             | Rear panel LEDs command. <br /> _[std_msgs/UInt8]_          |
| ✅  | ❌    | **`ranges`**           | Range sensor data. <br /> _[sensor_msgs/Range]_             |
| ✅  | ✅    | **`_imu/data`**        | Raw IMU data. <br /> _[sensor_msgs/Imu]_                    |
| ✅  | ✅    | **`_motors/cmd`**      | Wheel speed commands. <br /> _[std_msgs/Float32MultiArray]_ |
| ✅  | ✅    | **`_motors/feedback`** | Wheel feedback. <br /> _[sensor_msgs/JointState]_           |

## Services

[std_srvs/Trigger]: https://docs.ros.org/en/jazzy/p/std_srvs/srv/Trigger.html

| SERVICE       | DESCRIPTION                             |
| ------------- | --------------------------------------- |
| **`_mcu_id`** | Get MCU ID. <br /> _[std_srvs/Trigger]_ |
