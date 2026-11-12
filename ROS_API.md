# ROS API

The MCU's ROS 2 interface, advertised on the SBC by
[rosbot_mavlink_bridge], which translates the firmware's MAVLink link.
Downstream nodes (e.g. `rosbot_ros`) consume this node name, topic list,
types, namespacing and QoS; see [ARCHITECTURE.md](ARCHITECTURE.md) for the
wire side.

## Nodes

[rosbot_mavlink_bridge]: ./bridge/rosbot_mavlink_bridge

| NODE             | DESCRIPTION                                                                                                                                                                                       |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **`rosbot_mcu`** | Node exposing the ROSbot MCU's topics and services. Advertised by [rosbot_mavlink_bridge]. |

## Topics

[sensor_msgs/BatteryState]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/BatteryState.html
[sensor_msgs/Image]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/Image.html
[sensor_msgs/Imu]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/Imu.html
[sensor_msgs/JointState]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/JointState.html
[sensor_msgs/Range]: https://docs.ros.org/en/jazzy/p/sensor_msgs/msg/Range.html
[std_msgs/Float32MultiArray]: https://docs.ros.org/en/jazzy/p/std_msgs/msg/Float32MultiArray.html
[std_msgs/UInt8]: https://docs.ros.org/en/jazzy/p/std_msgs/msg/UInt8.html
[std_msgs/UInt8MultiArray]: https://docs.ros.org/en/jazzy/p/std_msgs/msg/UInt8MultiArray.html

| Rb  | Rb XL | TOPIC                  | DESCRIPTION                                                 |
| --- | ----- | ---------------------- | ----------------------------------------------------------- |
| ✅  | ✅    | **`battery`**          | Battery status. <br /> _[sensor_msgs/BatteryState]_         |
| ✅  | ✅    | **`buttons`**          | Button states. <br /> _[std_msgs/UInt8]_                    |
| ❌  | ✅    | **`led_strip`**        | LED strip command. <br /> _[sensor_msgs/Image]_             |
| ✅  | ✅    | **`leds`**             | Rear panel LEDs command. <br /> _[std_msgs/UInt8]_          |
| ✅  | ❌    | **`ranges`**           | Range sensor data. <br /> _[sensor_msgs/Range]_             |
| ✅  | ✅    | **`_imu/data`**        | Raw IMU data. <br /> _[sensor_msgs/Imu]_                    |
| ✅  | ✅    | **`_imu/calibration`** | BNO055 calibration status, 5 Hz — see below. <br /> _[std_msgs/UInt8MultiArray]_ |
| ✅  | ✅    | **`_motors/cmd`**      | Wheel speed commands. <br /> _[std_msgs/Float32MultiArray]_ |
| ✅  | ✅    | **`_motors/feedback`** | Wheel feedback. <br /> _[sensor_msgs/JointState]_           |

## Services

[std_srvs/Trigger]: https://docs.ros.org/en/jazzy/p/std_srvs/srv/Trigger.html

| SERVICE                       | DESCRIPTION                                                                  |
| ----------------------------- | ---------------------------------------------------------------------------- |
| **`_mcu_id`**                 | Get MCU ID. <br /> _[std_srvs/Trigger]_                                      |
| **`_power_board_info`**       | ROSbot XL only. Power board firmware and hardware version as JSON in `message`: `{"firmware_version": "v1.0.1", "board_version": "v1.4"}`. `success` is false (empty strings) when the power board has not reported yet or the robot has no power board, so a caller should retry for a few seconds after boot. <br /> _[std_srvs/Trigger]_ |
| **`_imu/start_calibration`**  | Start a calibration session (red LED fast blink, 180 s). <br /> _[std_srvs/Trigger]_ |
| **`_imu/stop_calibration`**   | End the session early. <br /> _[std_srvs/Trigger]_             |
| **`_imu/save_calibration`**   | Persist the chip's current offsets to flash. <br /> _[std_srvs/Trigger]_ |

## Battery

`battery` fields that need a note (ROSbot XL; ROSbot 2/3 keep NaN / UNKNOWN):

- `temperature` — pack temperature in Celsius from the power board NTC; NaN
  when the power board reports nothing usable.
- `power_supply_health` — `GOOD` with power board firmware >= v1.0.3 and no
  fault; `COLD` when charging is inhibited below 0 C; `UNSPEC_FAILURE` after
  charger I2C errors; `OVERHEAT`, `DEAD`; `UNKNOWN` with older power board
  firmware, an older bridge, or an unrecognised value.

On the wire this rides in `BATTERY_STATUS` `temperature` (cdegC) and
`charge_state` / `fault_bitmask`; no message change.

## IMU calibration

The BNO055 calibrates itself continuously; these entry points only show its
progress and persist the result, so the robot keeps driving throughout and
no MCU reset is needed.

`_imu/calibration` data layout:
`[sys, gyro, accel, mag, save_state, save_seq, has_saved, session]`.

- `sys`/`gyro`/`accel`/`mag` — the chip's CALIB_STAT, 0..3 each.
- `save_state` — result of the last save: 0 none, 1 saving, 2 saved,
  3 rejected (not calibrated), 4 failed (flash).
- `save_seq` — increments on every completed save attempt.
- `has_saved` — 1 when flash holds a calibration record.
- `session` — 1 while a session started by `_imu/start_calibration` is on.

Saving requires `gyro == accel == mag == 3`. `sys` is ignored: it is the
fusion's confidence, not an offset status, and it does not reliably reach 3
on an assembled robot. The session only drives the LED; a save is accepted
with or without one.

`_imu/save_calibration` replies `success` plus a JSON `message`:
`{"result": R, "sys": .., "gyro": .., "accel": .., "mag": ..}` with `R` one
of `saved`, `not_calibrated`, `failed`, `timeout`, `no_ack`, `no_status`.
Start/stop reply `{"result": "ok" | "rejected" | "no_ack"}`.
