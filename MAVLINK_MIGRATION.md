# MAVLink migration — implementation spec

> **Status: shipped.** Phases 1–5 are implemented on branch `jazzy-mavlink`.
> This document is preserved as the design record — rationale, dialect IDs,
> the D1–D24 sign-off decisions, and the phasing that got us here. It is no
> longer the live reference for *how the firmware works today*; for that see:
>
> - [ARCHITECTURE.md](ARCHITECTURE.md) — current firmware architecture,
>   including the MAVLink stack.
> - [ROS_API.md](ROS_API.md) — user-facing ROS 2 topic / service contract
>   (identical for both flavours).
> - [README.md](README.md) — how to build, flash and launch each flavour.
>
> Read this document when you need to know *why* a design decision was made,
> not *what* the current code does.

This document was the implementation contract for adding a MAVLink-based
communication stack to the firmware alongside the existing micro-ROS / XRCE-DDS
stack. All open design decisions had been resolved (see §2) before
implementation began.

Branch for the work: `jazzy-mavlink` (off `jazzy`). The micro-ROS path on `jazzy`
keeps shipping unchanged; the MAVLink path adds new PlatformIO envs and a parallel
`lib/mavlink/` subsystem.

Companion documents:
- [ARCHITECTURE.md](ARCHITECTURE.md) — current firmware architecture.
- [ROS_API.md](ROS_API.md) — user-facing topic / service contract. The bridge
  preserves this exactly. No diffs.
- [CLAUDE.md](CLAUDE.md) — workflow expectations.

---

## 1. Goals and non-goals

**Goals**

- Drop `micro_ros_agent` from the SBC dependency chain. The SBC runs vanilla ROS 2
  with the user's chosen RMW (FastDDS, CycloneDDS, Zenoh).
- Wire protocol between MCU and SBC becomes **MAVLink v2**.
- **Transport choice is invisible to downstream ROS 2 consumers.** The user-facing
  ROS 2 API — topic names, message types, namespace, node name, QoS — is
  byte-for-byte identical to what `micro_ros_agent` produces today. A consumer
  node (e.g. `rosbot_ros` in the snap) cannot tell whether the firmware is
  speaking micro-ROS or MAVLink. From the consumer's perspective there is no
  migration.
- The bridge node responsible for that translation ships in **this firmware
  repo**, alongside the firmware code that produces the MAVLink wire format.
  Both halves of the protocol are versioned and released together.
- **One firmware binary serves all SBC distros.** The MAVLink firmware has zero
  ROS-2 dependency in its source, so the same `.bin` runs against jazzy,
  humble, or any future distro on the SBC. This is a *natural consequence* of
  using MAVLink rather than micro-ROS — the existing micro-ROS firmware is
  jazzy-pinned by `micro_ros_arduino#v2.0.8-jazzy` and would need separate
  builds per distro; MAVLink removes that coupling.
- **One bridge source tree builds for both jazzy and humble** (and any future
  distro), via a CI matrix that compiles the same package in two different
  ROS-2 containers. The rclcpp API surface and message types we touch are
  stable between humble and jazzy; we expect no source split. If a future
  distro ever forces a divergence, we localise it behind a `#ifdef`
  rather than forking the package.
- Coexistence with the current micro-ROS path is **build-flag selectable** via new
  PlatformIO envs.
- No regression in latency, jitter, or CPU floor relative to the event-driven
  DMA-TX serial / LwIP-UDP transports.

**Non-goals**

- Compatibility with QGroundControl / mavros / ArduPilot ground stations as
  end-users. The dialect is *mavros-shaped* for naming familiarity only; we are
  not a flight controller.
- A third protocol option.
- Removing the FTDI diagnostic-serial pre-communication phase
  (`CommunicationManager::configureNamespace`). It is reused unchanged.
- Modifying `lib/<not-ros>/` libraries (motor, imu, battery, encoder, range, fan,
  led_strip, indicator, pid, eeprom, comm_manager, power_board).
- **Changing anything in `rosbot_ros`.** That repository's driver code stays as
  it is. All translation between the MAVLink wire and the public ROS 2 API
  happens inside the bridge node that lives in this firmware repo.

---

## 2. Decisions (signed off)

| # | Topic | Decision |
|---|---|---|
| D1 | Migration strategy | **Coexistence behind PlatformIO envs.** Both stacks live in the tree; per-env build flags select one. |
| D2 | Branch | `jazzy-mavlink` off `jazzy`. Merges back to `jazzy` at Phase 5. |
| D3 | Bridge location | **New ROS 2 package inside this firmware repo**, name `rosbot_mavlink_bridge`. The package exposes the same ROS 2 API as `micro_ros_agent` currently provides — topic names, message types, namespace, node name and QoS are unchanged. `rosbot_ros` and any downstream consumer require **zero** changes. Not mavros. |
| D4 | MAVLink C library | **Generate from `rosbot.xml`** (custom dialect extending `common.xml`) using `pymavlink mavgen.py`. Generated headers checked into `lib/mavlink/dialect/generated/` for hermetic builds. Same headers vendored by the bridge. |
| D5 | Env naming | `rosbot_mavlink`, `rosbot_mavlink_release`, `rosbot_xl_mavlink`, `rosbot_xl_mavlink_release`. Eight envs total in `platformio.ini`. |
| D6 | MAVLink identity | **sysid=1, compid=1** (`MAV_COMP_ID_AUTOPILOT1`). HEARTBEAT `type=MAV_TYPE_GROUND_ROVER`, `autopilot=MAV_AUTOPILOT_GENERIC`. |
| D7 | Dialect ID base | Custom messages start at **11000** (`ROSBOT_*`). Free block at the time of writing. |
| D8 | IMU shape | **Single custom `ROSBOT_IMU` message** carrying quaternion + gyro + accel + timestamp. No correlation logic on the bridge. |
| D9 | Joint state shape | Fixed **`float[4]`** for position / velocity / effort. Matches firmware `MAX_NUM_MOTORS=4`. |
| D10 | HEARTBEAT timing | **1 Hz send.** Declare `DISCONNECTED` after **3 s** without a peer HEARTBEAT. |
| D11 | MCU ID service | `COMMAND_LONG(MAV_CMD_USER_1)` → `COMMAND_ACK` → `ROSBOT_MCU_ID`. Bridge correlates, exposes as `std_srvs/Trigger` on `_mcu_id`. |
| D12 | Motor watchdog | **Keep existing 500 ms `MotorArray::feedWatchdog()`.** Fed when `ROSBOT_WHEEL_SETPOINTS` arrives. |
| D13 | Message signing | **Off** for both MVP and v1. Same trust model as today. |
| D14 | Diagnostic-serial logs | **Keep diag-serial logs and** emit duplicate `STATUSTEXT` over the main link. |
| D15 | Time sync | **MCU-initiated MAVLink `TIMESYNC`** ping-pong. Filtered offset applied to all timestamps. |
| D16 | rosbot serial transport | **Reuse DMA TX + yielding-poll RX** pattern from `lib/ros/ros/transport/serial_transport.cpp`, strip XRCE framing, add MAVLink parser. |
| D17 | rosbot_xl UDP transport | **Mavros default ports**: MCU listens on **14555**, sends to **14550**. LwIP raw API pattern reused. |
| D18 | CI matrix | **Build all 8 envs** on every push. Pre-commit job remains as is. |
| D19 | Mismatch detection | Firmware emits a `STATUSTEXT` boot banner `"rosbot[_xl] <FW_VERSION> mavlink"`. Bridge logs it once for confirmation but does **not** gate on it — the variant is already verified by `pre_communication` (FW:/BACKEND: handshake) before the bridge starts. CONNECTED is gated on the MCU HEARTBEAT. |
| D20 | Deprecation timeline | **Decided in Phase 4** after parity validation. Spec stays silent until then. |
| D21 | Telemetry rates | **Identical** to micro-ROS build (200 Hz joint state, 100 Hz IMU, 10 Hz ranges, 1 Hz battery, 20 Hz buttons). No `REQUEST_DATA_STREAM` mechanism. |

D22 (implicit): namespace handshake stays on FTDI; bridge reads namespace from
`rosbot-snap`-managed launch parameter. No MAVLink parameter exchange in MVP.

D23 (implicit, from D3 clarification): the bridge node uses the **same node
name** (`rosbot_mcu`) and the **same topic names with the same QoS** that the
existing micro-ROS firmware advertises today (see §4–§5 and `ROS_API.md`). The
goal is that running `ros2 topic list` and `ros2 topic info -v <topic>` against
the bridge produces output indistinguishable from `micro_ros_agent` output. This
is a non-negotiable acceptance criterion; if a wire-protocol detail forces a QoS
or topic-shape change, the bridge absorbs the difference rather than leaking it
upward.

D24: ROS-2-distro coverage. The MAVLink firmware binary is **distro-agnostic**
(one `.bin` per variant × debug/release works for any SBC distro). The bridge
package is **a single source tree built per supported distro** in CI; initial
release covers **jazzy and humble** out of one source. No code fork between
distros unless a hard API break appears; current scope confirms the rclcpp
surface and message types are stable between the two. Tagged release artefacts
make the split explicit: firmware `.bin` files carry no distro suffix; bridge
tarballs do.

---

## 3. The refactor seam — why this is cheap

The ROS-2 type boundary in the firmware is already narrow and local:

- Telemetry producers (`MotorArray`, `BatteryInterface`, `ImuInterface`,
  `RangeArray`) emit plain C structs (`MotorsData`, `BatteryData`, `ImuData`,
  `RangesData`) into FreeRTOS queues. They have **no ROS dependency**. Reuse 1:1.
- `sensor_msgs__msg__*` appears only inside `lib/ros/ros/publishers/*.hpp` and in
  subscriber callbacks in `src/<variant>/ros.cpp`. The MAVLink subsystem mirrors
  the file structure under `lib/mavlink/` and reads the same queues.
- Transport callbacks (`rmw_uros_set_custom_transport`) are already abstracted —
  the same code organisation pattern carries over to a `MavlinkTransport` base
  with `open / close / read / write`. XRCE framing is gone; MAVLink does its own
  framing + CRC.
- Pre-communication negotiation (`CommunicationManager`) is **protocol-agnostic**.
  Both stacks call `configureNamespace()` identically.
- `RosNode::loop()` state machine maps 1:1 to a `MavlinkNode::loop()` with
  HEARTBEAT-based connectivity. Same FreeRTOS task harness.

Untouched libraries: `battery`, `imu`, `motor`, `encoder`, `range`, `fan`,
`power_board`, `led_strip`, `pid`, `eeprom`, `indicator`, `comm_manager`.

Touched files (new): `lib/mavlink/**`, `src/<variant>/main_mavlink.cpp`,
`src/<variant>/ros_mavlink.cpp`, `bridge/rosbot_mavlink_bridge/**`.
Touched files (modified): `platformio.ini`, `.github/workflows/ci.yaml`,
`.github/workflows/release.yaml`. **No changes** to `lib/ros/**`. **No changes
to `rosbot_ros`** — its driver code is read-only for this feature; the bridge
in `bridge/` is responsible for hiding the wire-protocol switch from it.

---

## 4. Telemetry messages (MCU → SBC)

All timestamps are `time_boot_us` (monotonic since MCU boot). The bridge converts
to wall-clock using the offset learned via `TIMESYNC` (§6.2).

| ROS 2 topic | Rate | MAVLink wire | Payload size |
|---|---|---|---|
| `battery` | 1 Hz | `BATTERY_STATUS` (common, ID 147) | ~36 B |
| `_imu/data` | 100 Hz | `ROSBOT_IMU` (custom, ID 11001) | 48 B |
| `_motors/feedback` | 200 Hz | `ROSBOT_JOINT_STATE` (custom, ID 11002) | 56 B |
| `ranges` (rosbot) | 10 Hz × 4 sensors | `DISTANCE_SENSOR` (common, ID 132) | ~14 B each |
| `buttons` | 20 Hz | `ROSBOT_BUTTONS` (custom, ID 11003) | 9 B |

### 4.1 `BATTERY_STATUS` (common.xml ID 147)

Fields used:
- `id = 0`
- `battery_function = MAV_BATTERY_FUNCTION_ALL`
- `type = MAV_BATTERY_TYPE_LION`
- `voltages[10]` — pack cell voltages in mV. Unused slots = `UINT16_MAX`.
  rosbot/rosbot_xl have 3 cells: indices 0..2 used.
- `current_battery` — total current in cA (signed). `−1` if unknown.
- `battery_remaining` — percent (0..100, `−1` unknown).
- `temperature = INT16_MAX` (unknown).
- `current_consumed = energy_consumed = −1` (unknown).

Source on MCU: `g_battery->getData()`. The `BatteryData` struct already carries
voltage, current, percent — no new sensor code.

### 4.2 `DISTANCE_SENSOR` (common.xml ID 132)

One message per sensor per cycle, four sensors:
- `id = 0..3` mapped to `fl_range`, `fr_range`, `rl_range`, `rr_range` (table in
  the bridge).
- `min_distance / max_distance` in cm (1 cm / 90 cm matching `range_pub_config`).
- `current_distance` in cm.
- `type = MAV_DISTANCE_SENSOR_LASER`.
- `orientation` — set to `MAV_SENSOR_ROTATION_NONE`; the bridge ignores this and
  uses the `id`→`frame_id` table.
- Covariance unused (`UINT8_MAX`).

### 4.3 `ROSBOT_IMU` (custom, ID 11001)

```
uint64_t time_boot_us
float quaternion[4]      // {x, y, z, w}
float angular_velocity[3]  // [rad/s]  in IMU frame
float linear_acceleration[3]  // [m/s²] in IMU frame
```

Bridge fills `sensor_msgs/Imu` with these and sets `frame_id = "imu_link"`.
Covariances are filled with `-1.0` (per ROS convention: unknown).

### 4.4 `ROSBOT_JOINT_STATE` (custom, ID 11002)

```
uint64_t time_boot_us
float position[4]   // [rad]   wheel order: FL, FR, RL, RR
float velocity[4]   // [rad/s] same order
float effort[4]     // [Nm] or PWM-duty if no current source — see ARCHITECTURE.md §Patterns "Effort source dispatch"
```

Wheel name strings (`fl_wheel_joint` etc.) are a bridge-side constant; we do not
transmit them. The order is contractual.

### 4.5 `ROSBOT_BUTTONS` (custom, ID 11003)

```
uint64_t time_boot_us
uint8_t  mask
```

`mask` is the same bitmask the existing `ButtonsPublisher` exposes. Sent at 20 Hz
unconditionally (cheap, matches current behaviour).

---

## 5. Command messages (SBC → MCU)

| ROS 2 topic / service | MAVLink wire | Payload |
|---|---|---|
| `_motors/cmd` | `ROSBOT_WHEEL_SETPOINTS` (11010) | 24 B |
| `leds` | `ROSBOT_PANEL_LEDS` (11011) | 1 B |
| `led_strip` (rosbot_xl) | `ROSBOT_LED_STRIP` (11012) | 55 B |
| `_mcu_id` | `COMMAND_LONG(MAV_CMD_USER_1)` → `COMMAND_ACK` → `ROSBOT_MCU_ID` (11020) | per-msg |

### 5.1 `ROSBOT_WHEEL_SETPOINTS` (11010)

```
uint64_t time_boot_us
float velocity[4]   // [rad/s] FL, FR, RL, RR
```

On receipt: call `g_motors.setVelocities(msg.velocity)`. This already feeds the
500 ms watchdog (D12).

### 5.2 `ROSBOT_PANEL_LEDS` (11011)

```
uint8_t mask
```

On receipt: dispatch into the existing `ledsCallback` machinery in
`subscribers/led_subscriber.hpp`. Bitmask semantics identical to the current
`std_msgs/UInt8` topic.

### 5.3 `ROSBOT_LED_STRIP` (11012) — rosbot_xl only

```
uint8_t  count                     // 0..18
uint8_t  rgb[54]                   // 18 pixels × 3 bytes; only count*3 valid
```

On receipt: push a `LedFrameMsg` onto `led_strip_queue`. Bridge converts a
`sensor_msgs/Image` (height=1, encoding="rgb8", width≤18) into this message.

### 5.4 `_mcu_id` service mapping

1. Bridge: `COMMAND_LONG{command=MAV_CMD_USER_1, target_system=1, target_component=1}`.
2. MCU: `COMMAND_ACK{command=MAV_CMD_USER_1, result=MAV_RESULT_ACCEPTED}`.
3. MCU: `ROSBOT_MCU_ID{uid: char[24]}` (hex string, 24 chars).
4. Bridge correlates by source compid and returns the UID in the `Trigger`
   response as JSON: `{"mcu_id": "<24-char hex>"}` (matches current micro-ROS
   service payload exactly).

Retries: bridge re-issues `COMMAND_LONG` after 500 ms if no `COMMAND_ACK`
received, up to 3 attempts. Standard MAVLink command behaviour.

---

## 6. House-keeping messages

### 6.1 HEARTBEAT (common.xml ID 0)

Sent by MCU at 1 Hz. Fields:
- `type = MAV_TYPE_GROUND_ROVER`
- `autopilot = MAV_AUTOPILOT_GENERIC`
- `base_mode = 0`
- `custom_mode = 0`
- `system_status = MAV_STATE_ACTIVE` when CONNECTED, `MAV_STATE_STANDBY` otherwise

DISCONNECTED transition: no peer HEARTBEAT for 3 s → state goes to DISCONNECTED,
motor watchdog timeouts independently. Reconnect: any peer HEARTBEAT restarts the
handshake.

### 6.2 `TIMESYNC` (common.xml ID 111)

MCU initiates at 0.5 Hz with `tc1=0, ts1=mcu_now_ns`. Bridge replies with
`tc1=bridge_unix_ns, ts1=mcu_now_ns`. MCU computes:
```
offset_ns = (bridge_unix_ns − ts1_echoed) − rtt/2
filtered_offset = α·offset_ns + (1−α)·filtered_offset      // α=0.05
```
All `time_boot_us` → wall clock conversion happens on the **bridge** side using
the same offset. The MCU does not need to know wall-clock time.

### 6.3 `STATUSTEXT` (common.xml ID 253)

- One-shot on boot (D19): `"rosbot v1.1.0-jazzy mavlink"` or
  `"rosbot_xl v1.1.0-jazzy mavlink"`. `severity = MAV_SEVERITY_INFO`. Sent within
  100 ms of HEARTBEAT going active.
- Subsequent: every call to `MavlinkNode::log()` produces a `STATUSTEXT` with
  `severity = MAV_SEVERITY_DEBUG` (or higher for explicit error paths). Same
  string also goes to FTDI when `g_comm_mgr.hasDebugSerial()` (D14).

Bridge: forwards `STATUSTEXT` to ROS 2 `/rosout` via standard logger calls. The
boot banner is parsed by the bridge as an informational confirmation of firmware
type; it does not gate the CONNECTED transition (the MCU HEARTBEAT does).

---

## 7. Custom `rosbot` dialect

File: `lib/mavlink/dialect/rosbot.xml`. Includes `common.xml`. Initial content
(IDs in §4 and §5):

```xml
<?xml version="1.0"?>
<mavlink>
  <include>common.xml</include>
  <version>1</version>
  <dialect>11</dialect>

  <messages>
    <message id="11001" name="ROSBOT_IMU">
      <description>IMU sample (orientation + gyro + accel) from rosbot MCU.</description>
      <field type="uint64_t" name="time_boot_us">Timestamp since MCU boot [us].</field>
      <field type="float[4]"  name="quaternion">Orientation {x,y,z,w}.</field>
      <field type="float[3]"  name="angular_velocity">Gyro [rad/s], IMU frame.</field>
      <field type="float[3]"  name="linear_acceleration">Accel [m/s^2], IMU frame.</field>
    </message>

    <message id="11002" name="ROSBOT_JOINT_STATE">
      <description>Wheel joint state, order: FL, FR, RL, RR.</description>
      <field type="uint64_t" name="time_boot_us">Timestamp since MCU boot [us].</field>
      <field type="float[4]"  name="position">Wheel position [rad].</field>
      <field type="float[4]"  name="velocity">Wheel angular velocity [rad/s].</field>
      <field type="float[4]"  name="effort">Wheel effort [Nm or PWM duty fallback].</field>
    </message>

    <message id="11003" name="ROSBOT_BUTTONS">
      <description>User-button bitmask snapshot.</description>
      <field type="uint64_t" name="time_boot_us">Timestamp since MCU boot [us].</field>
      <field type="uint8_t"  name="mask">Pressed-button bitmask.</field>
    </message>

    <message id="11010" name="ROSBOT_WHEEL_SETPOINTS">
      <description>Wheel velocity setpoints, order: FL, FR, RL, RR.</description>
      <field type="uint64_t" name="time_boot_us">Bridge-side stamp for diagnostics [us].</field>
      <field type="float[4]"  name="velocity">Target wheel angular velocity [rad/s].</field>
    </message>

    <message id="11011" name="ROSBOT_PANEL_LEDS">
      <description>Rear-panel LED state bitmask.</description>
      <field type="uint8_t" name="mask">Desired LED bitmask.</field>
    </message>

    <message id="11012" name="ROSBOT_LED_STRIP">
      <description>LED strip RGB frame (rosbot_xl).</description>
      <field type="uint8_t"     name="count">Number of valid pixels (0..18).</field>
      <field type="uint8_t[54]" name="rgb">Pixel data, RGB triplets, count*3 valid bytes.</field>
    </message>

    <message id="11020" name="ROSBOT_MCU_ID">
      <description>Reply to MAV_CMD_USER_1 carrying the MCU unique identifier.</description>
      <field type="char[24]" name="uid">Hex-encoded 12-byte MCU UID.</field>
    </message>
  </messages>
</mavlink>
```

Regeneration command (documented in `lib/mavlink/dialect/README.md`):
```bash
python -m pymavlink.tools.mavgen \
  --lang=C --wire-protocol=2.0 \
  --output=lib/mavlink/dialect/generated/ \
  lib/mavlink/dialect/rosbot.xml
```
Generated headers are committed. Regeneration is required only when `rosbot.xml`
changes. CI verifies (Phase 1 follow-up) that the committed headers match a fresh
mavgen run.

---

## 8. Build-system changes

### 8.1 PlatformIO envs

Add to `platformio.ini`:

```ini
; ──────────────────────────────────────────────────────────
; ROSBOT — MAVLink build
; ──────────────────────────────────────────────────────────
[env:rosbot_mavlink]
extends = env
build_src_filter =
  +<rosbot/main_mavlink.cpp>
  +<rosbot/ros_mavlink.cpp>
  +<rosbot/rtos.cpp>
  -<rosbot/main.cpp>
  -<rosbot/ros.cpp>
  -<rosbot_xl/*>
build_flags =
    ${env.build_flags}
    -D ENABLE_HWSERIAL1
    -D ENABLE_HWSERIAL3
    -D ROSBOT
    -D USE_MAVLINK
    -I lib/mavlink/dialect/generated/rosbot

[env:rosbot_mavlink_release]
extends = env:rosbot_mavlink
build_unflags = -g
build_flags =
    ${env:rosbot_mavlink.build_flags}
    ${release_flags.build_flags}

; ──────────────────────────────────────────────────────────
; ROSBOT XL — MAVLink build
; ──────────────────────────────────────────────────────────
[env:rosbot_xl_mavlink]
extends = env
build_src_filter =
  +<rosbot_xl/main_mavlink.cpp>
  +<rosbot_xl/ros_mavlink.cpp>
  +<rosbot_xl/rtos.cpp>
  -<rosbot_xl/main.cpp>
  -<rosbot_xl/ros.cpp>
  -<rosbot/*>
build_flags =
    ${env.build_flags}
    -D ENABLE_HWSERIAL1
    -D ETHERNET_USE_FREERTOS
    -D LAN9303
    -D ROSBOT_XL
    -D USE_MAVLINK
    -I lib/mavlink/dialect/generated/rosbot

[env:rosbot_xl_mavlink_release]
extends = env:rosbot_xl_mavlink
build_unflags = -g
build_flags =
    ${env:rosbot_xl_mavlink.build_flags}
    ${release_flags.build_flags}
```

`micro_ros_arduino` is **not** removed from `lib_deps` — it is still needed for
the existing envs. The unused-symbol stripping in the linker discards micro-ROS
code from the MAVLink builds (verify in Phase 1 release-build size; mismatch
> 10 KB needs investigation).

### 8.2 Source tree

```
lib/
├── ros/                                     # unchanged
└── mavlink/                                 # NEW
    ├── mavlink_node.{hpp,cpp}               # equivalent of RosNode
    ├── publishers/
    │   ├── publisher_interface.hpp
    │   ├── battery_publisher.hpp
    │   ├── imu_publisher.hpp
    │   ├── joint_state_publisher.hpp
    │   ├── range_publisher.hpp
    │   └── buttons_publisher.hpp
    ├── subscribers/
    │   ├── wheel_cmd_subscriber.hpp
    │   ├── led_subscriber.hpp
    │   └── led_strip_subscriber.hpp
    ├── commands/
    │   └── mcu_id_command.hpp
    ├── transport/
    │   ├── mavlink_transport_interface.hpp
    │   ├── mavlink_serial_transport.{hpp,cpp}
    │   └── mavlink_udp_transport.{hpp,cpp}
    └── dialect/
        ├── README.md                         # mavgen regeneration recipe
        ├── rosbot.xml                        # source of truth
        └── generated/                        # checked-in mavgen output
            ├── common/
            └── rosbot/
                ├── mavlink.h
                ├── rosbot.h
                └── mavlink_msg_*.h

src/
├── rosbot/
│   ├── main.cpp           # untouched
│   ├── main_mavlink.cpp   # NEW — equivalent of main.cpp using MavlinkNode
│   ├── ros.cpp            # untouched
│   ├── ros_mavlink.cpp    # NEW — equivalent of ros.cpp wiring MavlinkNode
│   └── rtos.cpp           # untouched — same task harness for both builds
└── rosbot_xl/
    ├── main.cpp
    ├── main_mavlink.cpp   # NEW
    ├── ros.cpp
    ├── ros_mavlink.cpp    # NEW
    └── rtos.cpp           # untouched

bridge/                                       # NEW — SBC-side ROS 2 package
└── rosbot_mavlink_bridge/
    ├── package.xml                           # ament_cmake, ROS 2 jazzy
    ├── CMakeLists.txt
    ├── README.md                             # build + run instructions
    ├── include/rosbot_mavlink_bridge/
    │   ├── bridge_node.hpp
    │   ├── transport/
    │   │   ├── transport_interface.hpp
    │   │   ├── serial_transport.hpp
    │   │   └── udp_transport.hpp
    │   ├── publishers/                       # MAVLink → ROS 2
    │   │   ├── battery.hpp
    │   │   ├── imu.hpp
    │   │   ├── joint_state.hpp
    │   │   ├── range.hpp
    │   │   └── buttons.hpp
    │   ├── subscribers/                      # ROS 2 → MAVLink
    │   │   ├── wheel_cmd.hpp
    │   │   ├── leds.hpp
    │   │   └── led_strip.hpp
    │   ├── services/
    │   │   └── mcu_id.hpp
    │   ├── time_sync.hpp                     # TIMESYNC filter
    │   └── status_text.hpp                   # STATUSTEXT → /rosout
    ├── src/
    │   ├── bridge_node.cpp
    │   ├── main.cpp                          # rclcpp::spin entry
    │   └── transport/{serial,udp}_transport.cpp
    ├── launch/
    │   ├── rosbot.launch.py
    │   └── rosbot_xl.launch.py
    └── config/
        ├── rosbot.yaml                       # baudrate, port, namespace
        └── rosbot_xl.yaml
```

Two build systems in one repo: PlatformIO for firmware envs, colcon/ament_cmake
for the bridge package. They do not share build state. Pre-commit (already in
the tree) covers both — `clang-format` applies to C++ in both halves.

The bridge package reuses the generated MAVLink headers from
`lib/mavlink/dialect/generated/` — its CMakeLists.txt resolves them via a
relative include path. Single source of truth for the dialect.

`rtos.cpp` stays shared because tasks read FreeRTOS queues and call interfaces —
neither references micro-ROS nor MAVLink directly. The only place tasks touch the
ROS layer is `g_ros_node.loop()` in the `uRos` task; on the MAVLink build that
becomes `g_mavlink_node.loop()` (compile-time selection via `USE_MAVLINK` or a
shared global of an abstract base — see implementation note below).

Implementation note: define an abstract `RoboticsLink` base (`loop()`,
`isConnected()`, etc.) in `include/robotics_link.hpp`. Both `RosNode` and
`MavlinkNode` inherit from it. The `uRos` task in `rtos.cpp` holds an
`extern RoboticsLink& g_link;` reference. The variant's `main*.cpp` instantiates
the right concrete class. This keeps `rtos.cpp` truly shared between builds.
Alternatively, two `rtos.cpp` versions with `#ifdef USE_MAVLINK` blocks — uglier
but smaller blast radius.

### 8.3 CI matrix

Modify `.github/workflows/ci.yaml` to add two build jobs after the existing
pre-commit step:

```yaml
build-firmware:
  name: Build firmware
  runs-on: ubuntu-latest
  strategy:
    matrix:
      env:
        - rosbot
        - rosbot_release
        - rosbot_xl
        - rosbot_xl_release
        - rosbot_mavlink
        - rosbot_mavlink_release
        - rosbot_xl_mavlink
        - rosbot_xl_mavlink_release
  steps:
    - uses: actions/checkout@v4
    - run: pip install -U platformio
    - run: pio run -e ${{ matrix.env }}

build-bridge:
  name: Build bridge
  runs-on: ubuntu-latest
  strategy:
    matrix:
      distro: [jazzy, humble]
  container: ros:${{ matrix.distro }}-ros-base
  steps:
    - uses: actions/checkout@v4
    - run: |
        rosdep update
        rosdep install --from-paths bridge --ignore-src -y
        . /opt/ros/${{ matrix.distro }}/setup.sh
        colcon build --packages-select rosbot_mavlink_bridge
        colcon test --packages-select rosbot_mavlink_bridge --return-code-on-test-failure
```

### 8.4 Developer flash workflow on the ROSbot SBC

Day-to-day build + flash is driven by [`justfile`](justfile) at the repo root.
`just` is a project-scoped command runner already installed on the ROSbot
image; `just --list` shows the recipes. The relevant ones:

```
just build rosbot_mavlink         # PlatformIO build, one env
just build-mavlink                # all four MAVLink envs
just flash rosbot_xl_mavlink      # build + flash the connected robot
```

`just flash` shells into [`scripts/flash.sh`](scripts/flash.sh), which does
*not* reimplement the STM32 bootloader handshake. It delegates to
`ros2 run rosbot_utils flash_firmware` — the same battle-tested entry point
the [rosbot-snap](https://github.com/husarion/rosbot-snap)
`snap/local/flash_launcher.sh` uses in production. The wrapper:

- maps the PlatformIO env name to the `--robot-model` arg
  (`rosbot_xl*` → `rosbot_xl`, otherwise `rosbot`),
- auto-selects USB/FTDI for ROSbot XL (or when `SERIAL_TYPE_USB=True`,
  matching the snap's `driver.serial-type-usb` semantics),
- finds the FTDI tty by USB VID:PID `0403:6015` via `pyudev` (override with
  `SERIAL_PORT=/dev/ttyUSBx`),
- points `flash_firmware --file` at the freshly built
  `.pio/build/<env>/firmware.bin` instead of the binary bundled inside
  `rosbot_utils`. **This is the only difference** from the snap path: we
  flash what we just compiled, not a pre-baked release artefact.

What this implies for MAVLink phases:

- **Phase 1 onward** — `just flash rosbot_mavlink` and
  `just flash rosbot_xl_mavlink` are the canonical ways to push the new
  firmware onto a robot for testing. The bridge is started separately from
  the ROS 2 workspace (`ros2 launch rosbot_mavlink_bridge ...` once the
  package lives at `bridge/rosbot_mavlink_bridge/`).
- **Phase 4 parity validation** — the same `flash.sh` flashes both stacks
  back-to-back on the same robot; the side-by-side `ros2 topic info -v` diff
  in §12 runs against the same physical hardware in two consecutive runs.
- **Phase 5 release** — the workflow that builds the release artefacts is
  separate from this dev recipe; production flashing uses the snap, which
  pulls the released `.bin`. The dev recipe is for developer machines only.

Release workflow (`release.yaml`) adds the four MAVLink envs to the firmware
build matrix and builds the bridge tarball once per supported distro:

```
rosbot-v1.2.0-jazzy.bin                          # micro-ROS, jazzy-pinned, unchanged
rosbot_xl-v1.2.0-jazzy.bin                       # micro-ROS, jazzy-pinned, unchanged
rosbot_mavlink-v1.2.0.bin                        # MAVLink, distro-agnostic (no distro suffix)
rosbot_xl_mavlink-v1.2.0.bin                     # MAVLink, distro-agnostic
rosbot_mavlink_bridge-v1.2.0-jazzy-src.tar.gz    # bridge, per-distro
rosbot_mavlink_bridge-v1.2.0-humble-src.tar.gz   # bridge, per-distro
```

The dropped `-<distro>` suffix on the MAVLink firmware binaries is intentional
and visible at the release-page level: it tells operators that flashing the
same `.bin` works across both jazzy and humble SBCs. The micro-ROS firmware
binaries keep the `-jazzy` suffix because they are jazzy-pinned today.

---

## 9. Firmware-side state machine

`MavlinkNode::loop()`:

```
WAITING
  ─ send HEARTBEAT every 1 s
  ─ on any peer HEARTBEAT received → AWAIT_TIMESYNC

AWAIT_TIMESYNC
  ─ send TIMESYNC every 200 ms
  ─ on first TIMESYNC reply with valid tc1 → CONNECTED
  ─ on 5 s without progress → WAITING

CONNECTED
  ─ send HEARTBEAT @ 1 Hz, TIMESYNC @ 0.5 Hz, telemetry per §4 rates
  ─ react to commands per §5
  ─ if no peer HEARTBEAT for 3 s → DISCONNECTED

DISCONNECTED
  ─ stop telemetry (heartbeat continues)
  ─ motor watchdog has already stopped motors (500 ms cmd timeout, D12)
  ─ → WAITING
```

The `uRos` FreeRTOS task is renamed conceptually to `Link` but keeps the same
priority and frequency (200 Hz on rosbot, 1 kHz on rosbot_xl). Telemetry rates
are enforced internally by simple counters on the publish-tick, not by an rclc
timer.

---

## 10. Bridge node (in-repo, package `rosbot_mavlink_bridge`)

The bridge lives in this repo under `bridge/rosbot_mavlink_bridge/` (§8.2). It is
a plain ROS 2 C++ node, **no micro-ROS dependency**, no MAVLink CLI tooling
dependency, no `mavros`. Runs with any RMW the SBC has configured. **Single
source tree** for both jazzy and humble (D24); the CI matrix builds it twice
against the matching `ros:<distro>-ros-base` container (the slim official
image; we don't need the `-desktop` extras).

### 10.1 API parity contract (hard requirement)

The bridge must expose **exactly the same ROS 2 API** as the existing
`micro_ros_agent` + micro-ROS firmware combination produces today, **including
namespacing**. Concretely:

- **Node name**: `rosbot_mcu` (matches `NODE_NAME` in
  `include/<variant>/config.hpp`).
- **Namespace** — *load-bearing detail, easy to get wrong*: the current
  micro-ROS firmware advertises **every** publisher, subscriber and service
  inside a namespace negotiated at boot through the FTDI `FW / NS / ACK`
  handshake (`lib/comm_manager/communication_manager.cpp:91-108` →
  `RosNode::setNamespace()` → `rclc_node_init_default(node, name, ns, support)`).
  When the handshake yields `foo`, today's topics are `/foo/battery`,
  `/foo/_imu/data`, `/foo/_motors/feedback`, etc., and the node appears as
  `/foo/rosbot_mcu`. The bridge **must reproduce this prefix end-to-end**.

  Mechanics: the bridge is constructed with a `ros_namespace` launch
  parameter (default `""`). It builds the node with that namespace
  (`rclcpp::NodeOptions().arguments({"--ros-args", "-r",
  "__ns:=" + ns_param})`, or equivalent). All publishers, subscribers and
  services declared with relative names (`battery`, `_imu/data`, …) are then
  automatically prefixed by the rclcpp runtime — the same way `rcl` prefixes
  them in the micro-ROS firmware today. The bridge does **not** prepend the
  namespace manually; it relies on rclcpp to do it, which guarantees parity
  with the rcl-based micro-ROS path.

  Source of truth for the namespace string: the same place `rosbot-snap`
  already uses to write `NS:<value>` to the FTDI line during firmware boot.
  The snap launches the bridge with `ros_namespace:=<value>`. Outside the
  snap (manual lab runs), the user passes `--ros-args -p
  ros_namespace:=<value>`. Default empty namespace must also work end-to-end.

- **Topic names, types and QoS** (relative names, prefixed by the namespace):

  | Topic (relative) | Type | QoS | Direction |
  |---|---|---|---|
  | `battery` | `sensor_msgs/BatteryState` | best_effort, depth 1 | pub |
  | `buttons` | `std_msgs/UInt8` | best_effort, depth 1 | pub |
  | `_imu/data` | `sensor_msgs/Imu` | best_effort, depth 1 | pub |
  | `_motors/feedback` | `sensor_msgs/JointState` | best_effort, depth 1 | pub |
  | `ranges` (rosbot only) | `sensor_msgs/Range` | best_effort, depth 1 | pub |
  | `_motors/cmd` | `std_msgs/Float32MultiArray` | best_effort, depth 1 | sub |
  | `leds` | `std_msgs/UInt8` | best_effort, depth 1 | sub |
  | `led_strip` (rosbot_xl only) | `sensor_msgs/Image` | best_effort, depth 1 | sub |

- **Service**: `_mcu_id` (`std_srvs/Trigger`), also relative. Final FQN under
  namespace `foo` is `/foo/_mcu_id`. Response payload is
  `{"success": true, "message": "{\"mcu_id\": \"<24-char hex>\"}"}` — exact JSON
  shape the current firmware emits in `src/<variant>/ros.cpp`.

Worked example for sanity-check during implementation, namespace `rosbot1`:

```
/rosbot1/rosbot_mcu        # node
/rosbot1/battery
/rosbot1/buttons
/rosbot1/_imu/data
/rosbot1/_motors/cmd
/rosbot1/_motors/feedback
/rosbot1/ranges            # rosbot only
/rosbot1/leds
/rosbot1/led_strip         # rosbot_xl only
/rosbot1/_mcu_id           # service
```

Acceptance test: a side-by-side diff of
```
ros2 node info /rosbot1/rosbot_mcu
ros2 topic info -v /rosbot1/<each-topic>
ros2 service list | grep rosbot1
```
between an agent-based setup and a bridge-based setup must show **zero
differences** (node name, topic list, types, QoS profiles, service list). If
they differ, the bridge is wrong and the bug is on the bridge side — never on
the consumer side.

### 10.2 Internal responsibilities

1. Open the matching transport: serial on rosbot (921600), UDP on rosbot_xl
   (bind 14550, send to 14555 at the firmware's IP). Connection parameters come
   from a launch file in `bridge/rosbot_mavlink_bridge/launch/`.
2. Parse the incoming MAVLink stream; for each message in §4 publish on the
   corresponding ROS 2 topic with the QoS in §10.1.
3. Consider the link up on the first MCU HEARTBEAT (matching sysid); do not
   publish telemetry before it. The boot banner (§6.3) is logged once if seen
   but is not required — the firmware variant is verified earlier by
   `pre_communication`.
4. TIMESYNC: respond to MCU-initiated TIMESYNC requests with `tc1` set to the
   bridge's wall clock. Maintain a filtered MCU-boot-to-wall-clock offset (EWMA,
   α = 0.05 default, configurable). Stamp all published ROS messages with
   `mcu_time_boot_us + offset`.
5. Subscribe to the command topics per §5; serialise into MAVLink messages and
   write to the transport.
6. Expose `_mcu_id` per §5.4 by issuing `COMMAND_LONG(MAV_CMD_USER_1)` and
   awaiting `COMMAND_ACK` + `ROSBOT_MCU_ID`.
7. Forward `STATUSTEXT` to `/rosout` via `RCLCPP_INFO` / `RCLCPP_WARN` /
   `RCLCPP_ERROR` (severity mapping from MAVLink to rclcpp logger levels).
8. (Optional, off by default) Publish a private `<ns>/mcu_link_state`
   (`std_msgs/UInt8`) topic for in-cluster diagnostics. This is purely
   additive — `rosbot_ros` does not depend on it. Off by default to preserve the
   exact topic list of the micro-ROS path; flip the parameter on for debug.

### 10.3 Versioning

Bridge and firmware are released together from a single tag, e.g. `v1.2.0`
(MAVLink artefacts) and `v1.2.0-jazzy` (existing micro-ROS artefacts; the
release workflow drives both naming schemes from the same source tag). The
release workflow (§8.3) produces:

- two micro-ROS firmware `.bin` files (variants × jazzy), unchanged from today,
- two MAVLink firmware `.bin` files (variants), **no distro suffix**,
- one bridge source tarball **per supported ROS-2 distro** (initially jazzy
  and humble).

`rosbot-snap` vendors the bridge tarball that matches its target distro at the
same tag as the firmware binary it flashes. Because the MAVLink firmware is
distro-agnostic, the snap pairs *any* of the bridge tarballs at a given tag
with the *same* `rosbot_mavlink-<tag>.bin`. Mismatched bridge / firmware
versions are detected by parsing the boot banner against an expected-version
regex configured at bridge build time.

---

## 11. Phasing

Every phase ends with both micro-ROS envs still building and flashing. Each
phase corresponds to one or more PRs into `jazzy-mavlink`.

**Phase 1 — skeleton (firmware + bridge land together in this repo).**
- Add `lib/mavlink/` with `rosbot.xml` and generated headers (D4, D7, §7).
- Add `bridge/rosbot_mavlink_bridge/` package skeleton (ament_cmake, jazzy)
  with the namespace parameter wired up (§10.1) but no message handling yet.
- New PlatformIO envs (D5, §8.1) — empty `MavlinkNode` that opens transport and
  emits HEARTBEAT + boot STATUSTEXT (D6, D19, §6).
- Bridge opens the matching transport, parses HEARTBEAT, declares the node
  under the namespace from the launch parameter, refuses to enter CONNECTED
  until the boot banner is seen.
- CI extended to build all 8 firmware envs AND the bridge package on jazzy
  (D18, §8.3).
- Exit criterion: both `rosbot_mavlink` and `rosbot_xl_mavlink` envs flash;
  bridge starts, sees HEARTBEAT, advertises `/<ns>/rosbot_mcu` node with no
  topics yet (or only the optional `mcu_link_state` topic if that flag is on);
  `colcon build` is green.

**Phase 2 — telemetry path.**
- Implement `BATTERY_STATUS`, `ROSBOT_IMU`, `ROSBOT_JOINT_STATE`,
  `DISTANCE_SENSOR`, `ROSBOT_BUTTONS` on the firmware side.
- Implement TIMESYNC handshake (D15, §6.2).
- Bridge maps to ROS 2 topics under the active namespace, applies TIMESYNC
  offset to stamps.
- Exit criterion: with the same namespace as the micro-ROS firmware,
  `ros2 topic echo /<ns>/battery /<ns>/_imu/data /<ns>/_motors/feedback
  /<ns>/ranges /<ns>/buttons` returns shape-identical data to the micro-ROS
  build, validated side-by-side on the same hardware. Stamp drift < 5 ms over
  1 min.

**Phase 3 — command path.**
- Implement `ROSBOT_WHEEL_SETPOINTS`, `ROSBOT_PANEL_LEDS`, `ROSBOT_LED_STRIP`,
  `MAV_CMD_USER_1` (MCU ID).
- Bridge subscribes to `/<ns>/_motors/cmd`, `/<ns>/leds`, `/<ns>/led_strip`,
  exposes `/<ns>/_mcu_id` service.
- Exit criterion: `teleop_twist_keyboard` → `rosbot_ros` driver → bridge →
  firmware → wheels turn, with `rosbot_ros` unaware that the lower layer
  changed. LED panel and (on rosbot_xl) LED strip respond. The `_mcu_id`
  service returns a valid UID. Motor watchdog stops wheels 500 ms after teleop
  is killed (D12).

**Phase 4 — parity validation.**
- STATUSTEXT log forwarding wired up.
- Measure CPU floor, latency, jitter on both stacks side-by-side; document in
  ARCHITECTURE.md under "MAVLink build".
- Validate the firmware + bridge works against an SBC running CycloneDDS and
  Zenoh (each tested for at least 30 minutes of teleop + telemetry) with
  `rosbot_ros` checked out unmodified.
- Validate the **same firmware binary** flashes and runs against an SBC on
  humble and an SBC on jazzy (the bridge tarball differs, the `.bin` does
  not).
- Exit criterion: performance within 10 % of micro-ROS baseline; no warnings
  from either RMW; a `diff` of `ros2 node info` / `ros2 topic info -v` output
  between agent setup and bridge setup is empty, on both distros.

**Phase 5 — release.**
- Tag a release. Release workflow produces `rosbot_mavlink-<tag>.bin`,
  `rosbot_xl_mavlink-<tag>.bin`, the existing micro-ROS binaries, and the
  bridge source tarball (§10.3).
- Update `README.md` with a "Two firmware flavours" section describing when to
  flash which and how to launch the bridge.
- Decide on the micro-ROS deprecation horizon (D20).

No phase silently breaks the micro-ROS build. The two stacks are fully
independent at the source-file level.

---

## 12. Validation criteria

Phase-by-phase exit criteria are listed above. Cumulative acceptance for the
whole feature on `jazzy-mavlink` before merging into `jazzy`:

- All 8 PlatformIO envs build cleanly with `-Wall -Wextra -Wfatal-errors`.
- `pio run -e rosbot_mavlink_release` and `pio run -e rosbot_xl_mavlink_release`
  produce binaries within +20 % flash, +10 % RAM of the matching micro-ROS
  release builds.
- `colcon build --packages-select rosbot_mavlink_bridge` and `colcon test`
  pass cleanly under **both jazzy and humble**, from the same source tree
  (D24).
- **API parity**: a side-by-side `diff` of
  `ros2 node info`, `ros2 topic info -v`, and `ros2 service list` between an
  agent-based setup and a bridge-based setup (same namespace, same hardware)
  is empty.
- A 30-minute teleop session on each variant runs without dropped
  `_motors/feedback` samples (logged on the SBC side).
- TIMESYNC offset stabilises to ±1 ms over 5 minutes once warmed up.
- A user clones an **unmodified** `rosbot_ros`, sets `RMW_IMPLEMENTATION` to
  any of FastDDS / CycloneDDS / Zenoh, launches the bridge from this repo, and
  runs the standard teleop demo with the MAVLink firmware. No code changes
  required in `rosbot_ros`, no manual remapping, no warnings.

---

## 13. What this plan does NOT change

- `lib/<not-ros>/` libraries: battery, imu, motor, encoder, range, fan,
  power_board, led_strip, pid, eeprom, indicator, comm_manager.
- RTOS task model and priorities.
- Pin maps, motor parameters, PID gains.
- Pre-communication phase (`CommunicationManager`).
- DMA-TX serial / LwIP-UDP transport patterns — they are **renamed** (new file
  paths under `lib/mavlink/transport/`) and **stripped of XRCE framing**, but
  the FreeRTOS / DMA / IRQ-priority design is preserved verbatim.
- `ROS_API.md` user-facing contract.
- `lib/ros/**` — the micro-ROS subsystem is read-only for this feature.
- **`rosbot_ros`** — the SBC-side driver repo is **not touched at all**. Its
  authors do not need to read this document. The bridge inside this repo
  shoulders the entire compatibility burden.
- Any downstream ROS 2 consumer (user nodes, navigation stack, behaviour
  trees) — they see byte-identical topics and services, namespaced exactly as
  before.

Anything outside that list is in scope for review; if priorities shift, update
this spec before code lands.
