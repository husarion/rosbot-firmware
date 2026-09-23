# rosbot-firmware

STM32F4 firmware for ROSbot 3 and ROSbot XL. The MCU talks MAVLink v2
(`rosbot` dialect) to the SBC, where
[`rosbot_mavlink_bridge`](./bridge/rosbot_mavlink_bridge) turns it into the
ROS 2 API described in [ROS_API.md](./ROS_API.md). One `.bin` per robot
serves every ROS 2 distro the bridge is built for.

> **micro-ROS support was removed after v2.1.0-jazzy.** Up to that release
> the firmware could also run a micro-ROS (XRCE-DDS) stack against
> `micro_ros_agent`, chosen at boot by the `BACKEND:` handshake line. It is
> still in git history; the firmware now answers `BACKEND:microros` with
> `NAK`.

## Build and flash

Day-to-day workflow on the ROSbot SBC uses [`just`](./justfile):

```bash
just install-deps         # one-time: pymavlink + platformio in a venv
just build rosbot_xl      # one env; envs: rosbot[_xl], rosbot[_xl]_release
just build-all            # all four envs
just flash rosbot_xl      # builds and flashes via rosbot_utils
just mavgen               # regen MAVLink dialect headers
just --list               # every recipe
```

`just flash` wraps `ros2 run rosbot_utils flash_firmware`; see
[`CONTRIBUTING.md`](./CONTRIBUTING.md) and [`scripts/flash.sh`](./scripts/flash.sh).

## Run the SBC side

```bash
# rosbot_xl (Ethernet, mavros default ports)
ros2 launch rosbot_mavlink_bridge rosbot_xl.launch.py namespace:=rosbot

# rosbot (Serial)
ros2 launch rosbot_mavlink_bridge rosbot.launch.py namespace:=rosbot \
  --ros-args -p serial_port:=/dev/ttyS4
```

The bridge advertises the `/<ns>/rosbot_mcu` node that `rosbot_ros` (snap)
consumes.

## Internals

- [ARCHITECTURE.md](./ARCHITECTURE.md) — firmware architecture, RTOS task
  layout, transport patterns, MAVLink stack.
- [ROS_API.md](./ROS_API.md) — ROS topic / service contract.
