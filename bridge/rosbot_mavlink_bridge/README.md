# rosbot_mavlink_bridge

ROS 2 bridge that talks MAVLink to the firmware (`rosbot_mavlink` /
`rosbot_xl_mavlink` PlatformIO envs) and exposes the **same** topics and
services that the existing micro-ROS firmware advertises today. From a
downstream consumer's perspective (e.g. the `rosbot_ros` driver inside
[rosbot-snap](https://github.com/husarion/rosbot-snap)), there is no
migration: same node name, same namespace, same QoS — only the wire format
changes.

See [`MAVLINK_MIGRATION.md`](../../MAVLINK_MIGRATION.md) for the full
implementation spec.

## Build

The bridge is a regular `ament_cmake` package and is **self-contained** —
the MAVLink dialect headers it needs are the canonical
[`mavlink_dialect/`](./mavlink_dialect/) tree shipped inside the package.
`just mavgen` (from the repo root) regenerates them in place; the
firmware build (PlatformIO) reads from the same directory via its
include path. Single source of truth, no duplication.

```bash
# from the repo root
colcon build --packages-select rosbot_mavlink_bridge
. install/setup.bash
```

The CI matrix builds the same source tree against both `jazzy` and
`humble` containers (see `.github/workflows/ci.yaml` — D24).

### Apt install (rosdistro)

Once released through bloom, the bridge is available as
`ros-<distro>-rosbot-mavlink-bridge` and is pulled in automatically as an
`exec_depend` of `rosbot_bringup`. Users on apt do not need this repo —
the bridge ships alongside `rosbot_ros`.

## Run

ROSbot XL (UDP transport):

```bash
ros2 launch rosbot_mavlink_bridge rosbot_xl.launch.py namespace:=rosbot
```

ROSbot (serial transport — pick the SBC's SBC<->MCU UART):

```bash
ros2 launch rosbot_mavlink_bridge rosbot.launch.py namespace:=rosbot \
  --ros-args -p serial_port:=/dev/ttyS4
```

The bridge waits for the firmware's boot banner
(`rosbot[_xl] <version> mavlink`) before declaring itself CONNECTED and
publishing telemetry — that's the D19 mismatch detector.

## Topics / services

| Path (relative to namespace) | Type | QoS |
|---|---|---|
| `battery` | `sensor_msgs/BatteryState` | best_effort, depth 1 |
| `_imu/data` | `sensor_msgs/Imu` | best_effort, depth 1 |
| `_motors/feedback` | `sensor_msgs/JointState` | best_effort, depth 1 |
| `buttons` | `std_msgs/UInt8` | best_effort, depth 1 |
| `ranges` (rosbot only) | `sensor_msgs/Range` | best_effort, depth 1 |
| `_motors/cmd` | `std_msgs/Float32MultiArray` | best_effort, depth 1 |
| `leds` | `std_msgs/UInt8` | best_effort, depth 1 |
| `led_strip` (rosbot_xl only) | `sensor_msgs/Image` | best_effort, depth 1 |
| `_mcu_id` (service) | `std_srvs/Trigger` | — |

`ros2 node info /<ns>/rosbot_mcu` and `ros2 topic info -v /<ns>/<topic>`
output is byte-identical between this bridge and the micro-ROS agent — that
is the Phase-4 acceptance criterion.

## Parameters

See `config/rosbot.yaml` and `config/rosbot_xl.yaml` for variants. Key
parameters:

- `transport`: `udp` | `serial`
- `peer_ip` / `peer_port` / `local_port` — UDP only (mavros default ports)
- `serial_port` / `serial_baudrate` — serial only
- `ros_namespace` — prefixed onto every advertised topic/service/node name
- `enable_ranges`, `enable_led_strip` — variant gates
- `publish_link_state` — if true, advertise an extra `mcu_link_state`
  `std_msgs/UInt8` topic for diagnostics (off by default for API parity)
- `timesync_alpha` — EWMA factor for the time-offset filter (D15)
- `expected_banner_regex` — boot-banner gate string
