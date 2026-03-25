# ROS API

## Nodes

[micro_ros_agent/micro_ros_agent]: https://github.com/micro-ROS/micro-ROS-Agent

| NODE             | DESCRIPTION                                                                                                  |
| ---------------- | ------------------------------------------------------------------------------------------------------------ |
| **`rosbot_mcu`** | Micro-ROS node responsible for communication with the ROSbot MCU. <br /> _[micro_ros_agent/micro_ros_agent]_ |

## Topics

[sensor_msgs/BatteryState]: https://docs.ros2.org/foxy/api/sensor_msgs/msg/BatteryState.html
[sensor_msgs/Image]: https://docs.ros2.org/foxy/api/sensor_msgs/msg/Image.html
[sensor_msgs/Imu]: https://docs.ros2.org/foxy/api/sensor_msgs/msg/Imu.html
[sensor_msgs/JointState]: https://docs.ros2.org/foxy/api/sensor_msgs/msg/JointState.html
[std_msgs/Float32MultiArray]: https://docs.ros2.org/foxy/api/std_msgs/msg/Float32MultiArray.html
[std_msgs/UInt8]: https://docs.ros2.org/foxy/api/std_msgs/msg/UInt8.html

| Rb  | Rb XL | TOPIC                  | DESCRIPTION                                                 |
| --- | ----- | ---------------------- | ----------------------------------------------------------- |
| ✅  | ✅    | **`battery`**          | Battery status. <br /> _[sensor_msgs/BatteryState]_         |
| ✅  | ✅    | **`buttons`**          | Button states. <br /> _[std_msgs/UInt8]_                    |
| ❌  | ✅    | **`led_strip`**        | LED strip command. <br /> _[sensor_msgs/Image]_             |
| ✅  | ✅    | **`leds`**             | Rear panel LEDs command. <br /> _[std_msgs/UInt8]_          |
| ✅  | ❌    | **`ranges`**           | Range sensor data. <br /> _[sensor_msgs/Range]_             |
| ✅  | ✅    | **`_imu`**             | Raw IMU data. <br /> _[sensor_msgs/Imu]_                    |
| ✅  | ✅    | **`_motors_cmd`**      | Wheel speed commands. <br /> _[std_msgs/Float32MultiArray]_ |
| ✅  | ✅    | **`_motors_response`** | Wheel feedback. <br /> _[sensor_msgs/JointState]_           |

## Services

[std_srvs/Trigger]: https://docs.ros2.org/foxy/api/std_srvs/srv/Trigger.html

| SERVICE       | DESCRIPTION                             |
| ------------- | --------------------------------------- |
| **`_mcu_id`** | Get MCU ID. <br /> _[std_srvs/Trigger]_ |
