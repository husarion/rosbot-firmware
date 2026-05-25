# rosbot-firmware

STM32F4 firmware for ROSbot 3 and ROSbot XL. Ships in **two flavours** —
classic **micro-ROS** and the newer **MAVLink + bridge** — that expose the
same ROS 2 API to downstream consumers.

## Two firmware flavours

| Aspect | `rosbot[_xl]` (micro-ROS) | `rosbot[_xl]_mavlink` (MAVLink) |
|---|---|---|
| Wire protocol | XRCE-DDS | MAVLink v2 (`rosbot` dialect, [spec](./MAVLINK_MIGRATION.md)) |
| SBC side | `micro_ros_agent` | [`rosbot_mavlink_bridge`](./bridge/rosbot_mavlink_bridge) |
| SBC distro coupling | jazzy-pinned by `micro_ros_arduino` | distro-agnostic — one `.bin` for every ROS 2 distro the bridge ships for |
| ROS 2 API | `rosbot_mcu` node, topics in `ROS_API.md` | **identical** node name, topics, QoS — downstream nodes can't tell |
| Status | shipping (default) | shipping alongside on `jazzy` |

Pick the MAVLink flavour to drop the `micro_ros_agent` dependency on the SBC
and run the firmware against any ROS 2 distro (jazzy, humble, future). Pick
the micro-ROS flavour to stay on the known-good path. Switching is just a
re-flash + restart of the SBC-side process.

## Build and flash

Day-to-day workflow on the ROSbot SBC uses [`just`](./justfile):

```bash
just install-deps           # one-time: pymavlink + platformio in a venv
just build rosbot_xl        # micro-ROS variant
just build rosbot_xl_mavlink  # MAVLink variant
just flash rosbot_xl_mavlink  # builds and flashes via FTDI
just mavgen                  # regen MAVLink dialect headers
just --list                  # see every recipe
```

`just flash` wraps `ros2 run rosbot_utils flash_firmware`; see
[`CONTRIBUTING.md`](./CONTRIBUTING.md) and [`scripts/flash.sh`](./scripts/flash.sh)
for the details and PlatformIO env names.

## Run the SBC side

**micro-ROS:** start the agent against the firmware's transport.

```bash
# rosbot_xl (Ethernet)
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888

# rosbot (Serial)
ros2 run micro_ros_agent micro_ros_agent serial --dev <serial_port> --baud 921600
```

**MAVLink:** launch the bridge from this repo (or the released tarball).

```bash
# rosbot_xl (Ethernet, mavros default ports)
ros2 launch rosbot_mavlink_bridge rosbot_xl.launch.py namespace:=rosbot

# rosbot (Serial)
ros2 launch rosbot_mavlink_bridge rosbot.launch.py namespace:=rosbot \
  --ros-args -p serial_port:=/dev/ttyS4
```

Either path advertises the same `/<ns>/rosbot_mcu` node with the same topic
list, types and QoS — `rosbot_ros` (snap) consumes it unchanged.

## Internals

- [ARCHITECTURE.md](./ARCHITECTURE.md) — firmware architecture, RTOS task
  layout, transport patterns, MAVLink stack overview.
- [ROS_API.md](./ROS_API.md) — user-facing ROS topic / service contract
  (true for both flavours).
- [MAVLINK_MIGRATION.md](./MAVLINK_MIGRATION.md) — implementation spec for
  the MAVLink stack, dialect IDs, phasing.
