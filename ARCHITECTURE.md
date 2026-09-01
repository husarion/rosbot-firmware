# Architecture

Technical reference for the rosbot / rosbot_xl firmware. Companion to
[CLAUDE.md](CLAUDE.md), which covers workflow.

---

## Hardware

| | rosbot | rosbot_xl |
|---|---|---|
| MCU | STM32F407ZGT6, 168 MHz | STM32F407ZGT6, 168 MHz |
| RAM | 128 KB SRAM + 64 KB CCM | 128 KB SRAM + 64 KB CCM |
| Flash | 1 MB | 1 MB |
| Transport to SBC | UART (Serial1, PA10/PA9 @ 921600) | Ethernet via LAN9303 switch |
| IMU | BNO055 on dedicated I2C3 (PC9/PA8) | BNO055 on shared I2C2 (PF0/PF1) |
| Range sensors | 4× VL53L0X on dedicated `range_i2c` | none |
| Motor driver | DRV8848 (dual H-bridge) | rev 1.1: DRV8870, rev 1.2: MAX22205 |
| Current sense | none (back-EMF model only) | rev 1.2 only: MAX22205 CSO |
| Battery sense | ADC divider (`BatteryAdc`) | UART to PowerBoard MCU (`PowerBoard`) |
| Fan | none | rev 1.1 always-on, rev 1.2 PWM proportional |
| Encoders | 4× quadrature, 48 CPR, 34:1 gearbox | 4× quadrature, 64 CPR, 50:1 gearbox |

DMA1 cannot reach CCM RAM on the F4. **DMA buffers must live in regular
SRAM.** Static globals in anonymous namespaces with `alignas(4)` work.

LAN9303 is a 3-port managed L2 switch. The MCU connects to one of its
ports via RMII; the SBC and an external RJ45 jack hang off the other two.

### LAN9303 — port map, management access, and the VLAN-isolation trade-off

**Port map** (rosbot_xl only — confirmed against the schematic): Port 0 is
RMII, no magnetics, direct to the MCU (`ETH_TXD0/1`, `ETH_RXD0/1`,
`ETH_TXEN`, `ETH_CRSDV`) — this is `CLIENT_IP`/192.168.77.3. Port 1 goes
through magnetics to the SBC. Port 2 goes through separate magnetics to the
external RJ45 jack on the chassis. Factory default is an unmanaged flat
bridge across all three — no VLAN, no port isolation.

**Management access exists on this board, unused by the firmware today.**
Two independent paths reach the switch fabric's CSRs, both confirmed wired
on the schematic:
- **MDIO/SMI** — pins 21/20 (`MDIO`/`MDC`) land on nets `ETH_MDIO`/`ETH_MDC`,
  same prefix as the RMII bus to the MCU, i.e. the same pins
  `STM32Ethernet`'s `ethernetif.cpp` already uses for ordinary PHY register
  reads (`PHY_BSR`/`PHY_BCR` via `LAN9303_To_SMI_Address_Conv`). SMI reuses
  the MDIO/MDC pins with **extended addressing** (PHY address 16–31, vs.
  plain MIIM 0–15) to reach *all* internal registers, not just the Port 1/2
  PHYs — see LAN9303/LAN9303i datasheet (Microchip DS00002308A) §10.2. The
  address-conversion formula, confirmed against `ethernetif.cpp` and
  hand-verified against the datasheet: for a system-register byte offset
  `off`, `SMI_PHY_ADDR = 0x10 | ((off >> 6) & 0xF)`,
  `SMI_REG_ADDR = ((off >> 1) & 0x1F) | word_select` (`word_select` is 0 for
  the low 16 bits of the 32-bit register, 1 for the high 16 bits — two SMI
  transactions per 32-bit register).
- **I2C** — `EE_SDA`/`EE_SCL` (pins 35/36) sit on the MCU's `I2C2`
  (PF0/PF1) — the **same bus** the BNO055 IMU and the board-revision EEPROM
  already use (`ARCHITECTURE.md` table above, `BoardRevision` class). An
  EEPROM lives on that bus (all revisions except the very oldest).

**Register access, two levels of indirection** (datasheet §13.2.4.4-5,
§13.4.3): `SWITCH_CSR_DATA` (system-register byte offset `0x1AC`) and
`SWITCH_CSR_CMD` (`0x1B0`, bit 31 `CSR_BUSY`, bit 30 `R_nW`, bits 15:0
`CSR_ADDR`) are reached directly via the SMI formula above, and are
themselves used to indirectly read/write the actual Switch Fabric CSRs
(VLAN table, ingress config, etc. — Table 13-14 in the datasheet), addressed
by a *second*, independent 16-bit index (e.g. `SWE_VLAN_CMD` = `0x180B`,
`SWE_GLB_INGRESS_CFG` = `0x1840`, `SWE_PORT_INGRESS_CFG` = `0x1841`): write
`SWITCH_CSR_DATA`, then `SWITCH_CSR_CMD` with `CSR_BUSY` set and the target
index, poll until `CSR_BUSY` clears.

**VLAN table** (datasheet §6.4.4, §13.4.3.8-11): 16 slots, each holding a
VID plus a member/untag bit per port (bits 17/16 = Port 2 member/untag,
15/14 = Port 1, 13/12 = Port 0). Port-based (untagged) operation forcible
via the "802.1Q VLAN Disable" bit in `SWE_GLB_INGRESS_CFG`, which makes the
switch use each port's PVID instead of any 802.1Q tag — relevant since none
of Port 0/1/2's traffic is ever tagged today.

**What was tried and reverted (2026-08-27, branch
`feature/lan9303-vlan-isolation`, never merged):** `192.168.77.2` is
hardcoded identical on every rosbot_xl unit — deliberate, since the MCU
needs a fixed address for the SBC. Because the factory-default flat bridge
puts Port 0/1/2 on one broadcast domain, that address (and the MCU's
`192.168.77.3`) leaks onto Port 2 — confirmed on hardware as a real ARP
conflict (`NetworkManager`: *"IP address 192.168.77.2 cannot be configured
because it is already in use... by host ..."*) when a second rosbot_xl
shares the same external switch. VLAN 1 = {Port 0, Port 1} / VLAN 2 =
{Port 2} via the mechanism above cleanly stops the leak (verified: the
conflict is gone, the MAVLink bridge over Port 0↔Port 1 keeps working
normally since the MCU and SBC stay in the same VLAN) — **but simple
port-based VLAN can't do this selectively.** Isolating Port 2 from Port 1
also cuts the SBC off from *all* external connectivity through the RJ45
jack, not just the `192.168.77.0/24` leak — the SBC's own DHCP-assigned
address, and the documented "plug a laptop directly into the robot and
reach it at `192.168.77.2`" workflow (`husarion-os`'s `husarion-eth-mode`,
which runs `isc-dhcp-server` on that exact assumption), both stop working
the moment isolation is active, since Port 1 and Port 2 no longer bridge at
all. Confirmed on hardware (2026-08-27): after flashing, the SBC's wired
interface never acquires a DHCP lease from the external network again.

**The correct fix, not yet built:** make Port 1 an 802.1Q trunk — tagged
member of VLAN 1 (for MCU traffic) *and* untagged/native member of VLAN 2
(for everything else), via the LAN9303's "Hybrid" port mode
(`BM_EGRSS_PORT_TYPE`, datasheet §13.2.x — not yet looked up in detail).
That needs a matching change on the SBC side (`husarion-os`): a tagged VLAN
sub-interface (e.g. `enP8p1s0.1`) carrying `192.168.77.2`, with the plain
untagged interface going back to ordinary DHCP-only for external
connectivity. Firmware-only work can't finish this — it's a coordinated
change across `rosbot-firmware` and `husarion-os`. The reverted branch
(`feature/lan9303-vlan-isolation`) has a working, hardware-verified
`lib/lan9303/` driver (SMI/CSR access + VLAN table writes, with a
verify-before-enforce safety invariant — see the file's own comments) that
implements the *simple* (non-trunk) isolation; it's a reasonable starting
point for the register-access plumbing if someone builds the trunk version,
but its `isolateExternalPort()` call in `main.cpp` should NOT be re-enabled
as-is given the trade-off above.

---

## Variant model — how the dispatcher works

`platformio.ini` defines four envs: `rosbot`, `rosbot_release`, `rosbot_xl`,
`rosbot_xl_release`. Each variant's env sets:

- `build_src_filter` to include only `src/<variant>/*.cpp`
- `-D ROSBOT` or `-D ROSBOT_XL` macro
- Variant-specific transport flags (`-D ENABLE_HWSERIAL1`,
  `-D ETHERNET_USE_FREERTOS`, `-D LAN9303`, ...)

`include/config.hpp` is a thin dispatcher that includes
`include/<variant>/config.hpp` based on the macro.

Why this matters: **anything in `lib/`, `include/config.hpp`, or
`platformio.ini` affects both variants.** Anything in `src/<variant>/` or
`include/<variant>/` is variant-only.

For runtime board-revision selection (rosbot_xl rev 1.1 vs 1.2), the MCU
reads a string from EEPROM at boot via the `BoardRevision` class. Pin
configuration that depends on revision (current-limit pins, current-sensor
enable, fan mode) is applied at runtime in `setMaxMotorsCurrent(rev)` and
`setupCurrentSense(rev)` patterns in `src/rosbot_xl/main.cpp`.

---

## Software stack

Layers, bottom-up:

1. **STM32 HAL + LL** (vendor) — peripheral drivers. Used directly when we
   need DMA / interrupt control beyond what the framework exposes.
2. **stm32duino Arduino core** — `framework-arduinoststm32` (Husarion
   fork). Provides `Arduino.h`, `Wire`, `HardwareSerial`, `HardwareTimer`,
   `STM32Ethernet`, `LwIP`. Pinned via `platformio.ini`.
3. **STM32FreeRTOS 10.3.3** — single-core preemptive scheduler.
4. **Vendor / device libraries** — Adafruit_BNO055, Adafruit_BusIO,
   Pololu VL53L0X, etc.
5. **micro-ROS** — `micro_ros_arduino v2.0.8-jazzy` (rcl, rclc, rmw,
   xrce-dds-client). Custom transport layer in `lib/ros/ros/transport/`.
6. **Project libraries** in `lib/`.
7. **Variant entry** in `src/<variant>/`.

---

## Library layer (`lib/`)

| dir | role |
|---|---|
| `battery/` | `BatteryInterface` + `BatteryAdc` (rosbot battery via ADC) |
| `boot_option/` | `resolveBootAction()` — classifies the power-on button gesture (tap vs 3 s hold) into "change transport" / "calibrate IMU" / nothing, owns the confirmation LEDs — see "IMU calibration" under "Patterns" |
| `comm_manager/` | `CommunicationManager` — chooses primary vs diagnostic transport at boot, driven by `boot_option`'s decision |
| `eeprom/` | I2C EEPROM driver + `BoardRevision` (revision string read on rosbot_xl) |
| `encoder/` | `EncoderInterface` + `HardwareEncoder` (STM32 timer in encoder mode, x4) |
| `fan/` | Fan + NTC thermistor (rosbot_xl only) |
| `imu/` | `ImuInterface` + `ImuBno055` (BNO055, DMA path — see "Patterns"). Also exposes on-chip calibration status/offsets, persisted via `persistent_config` — see "IMU calibration" under "Patterns" |
| `indicator/` | Status LED state machine |
| `led_strip/` | APA102-style LED strip over SPI (rosbot_xl only) |
| `motor/` | `MotorInterface`, `MotorHiZ`, `MotorArray` (Hi-Z PWM control) |
| `persistent_config/` | Comm backend/namespace + BNO055 calibration offsets, stored as one record in flash sector 11. `save()` must run before the scheduler starts (sector erase stalls 1-3 s) — see "IMU calibration" |
| `pid/` | PID controller with feedforward, anti-windup, dead-zone boost |
| `power_board/` | UART protocol to rosbot_xl power board MCU (battery state) |
| `range/` | `RangeInterface` + `RangeVl53l0x` + `RangeArray` (rosbot only) |
| `ros/ros/` | micro-ROS node, publishers, subscribers, services, transports |
| `mavlink/` | MAVLink stack: `MavlinkNode`, publishers/subscribers, transports, `rosbot` dialect (see "MAVLink build") |

Each `*Interface` is the abstract base; the implementation file follows
the pattern `<noun>_<adjective>.{hpp,cpp}` (e.g. `motor_hi_z`, not
`hi_z_motor`).

`*Array` aggregates N pointers to interface, calls `init`/`update` on each
and exposes a flat `*Data` snapshot. `MotorArray` adds FreeRTOS mutex,
watchdog, and driver-group enable/disable on top of the basic pattern.
There is no `EncoderArray` — motors own their encoders directly (see
"Patterns: motor owns encoder").

Layering rule: **`lib/X` does not include `lib/Y` headers unless `Y`
provides a primitive `X` literally needs.** Concrete examples that are
intentionally NOT in libraries:

- `lib/motor` does not depend on `lib/battery` for supply voltage. `main.cpp`
  bridges via a free-function pointer registered with
  `MotorHiZ::setSupplyVoltageProvider`.
- `lib/imu` does not depend on `lib/comm_manager`.

Battery is allowed as an `extern BatteryInterface*` in
`battery_interface.hpp` because it's the agreed shared abstraction; the
concrete instance is selected per variant in main.

---

## RTOS task model

Defined in `src/<variant>/rtos.cpp`. Priority enum in `include/rtos.hpp`:

```
BLOCKING = 1   (lowest)
OBSERVING = 2
SENSORS = 3
COMMUNICATION = 4
CONTROL = 5    (highest)
```

### rosbot tasks

| name | priority | freq [Hz] | role |
|---|---|---|---|
| Battery | SENSORS | 10 | ADC sample + queue |
| Imu | SENSORS | 100 | DMA-read BNO055, queue `ImuStamped` |
| LedIndicator | OBSERVING | 20 | Status LED blink/solid logic |
| Monitor | BLOCKING | 1 | `vTaskGetRunTimeStats` to debug serial (debug builds only) |
| MotorControl | CONTROL | 200 | Encoder + PID + PWM + current/back-EMF estimation |
| Range | SENSORS | 10 | Read 4× VL53L0X |
| uRos | COMMUNICATION | 200 | `g_ros_node.loop()` — micro-ROS state machine + spin |

### rosbot_xl tasks

| name | priority | freq [Hz] | role |
|---|---|---|---|
| HwMonitor | OBSERVING | 10 | Battery (via PowerBoard UART) + fan + LED status, rate-limited internally |
| Imu | SENSORS | 100 | same as rosbot |
| LedStrip | COMMUNICATION | 30 | Drain `led_strip_queue`, render via SPI |
| Monitor | BLOCKING | 1 | Runtime stats (debug builds) |
| MotorControl | CONTROL | 200 | same as rosbot |
| Shutdown | OBSERVING | 3 | Detect graceful shutdown signal from power board, stop scheduler |
| uRos | COMMUNICATION | 1000 | same as rosbot |

`uRos` runs at 1000 Hz on rosbot_xl, 200 Hz on rosbot. The freq is the
upper bound on `vTaskDelayUntil` — the actual rate is throttled by the
transport's blocking `read()` (see "Patterns: micro-ROS transport").

Queue depths are 1 (`xQueueOverwrite`) for telemetry — newest sample
wins, no buffering. Watchdog on `MotorArray` stops motors after 500 ms
without `setVelocities()` (in `feedWatchdog()`).

---

## ROS interface

User-facing contract is in [ROS_API.md](ROS_API.md). Implementation map:

- Publishers in `lib/ros/ros/publishers/` — one per topic.
- Subscribers in `lib/ros/ros/subscribers/` (or in
  `src/<variant>/ros.cpp` when variant-specific, e.g. `led_strip` only on
  rosbot_xl).
- Services in `src/<variant>/ros.cpp` (currently only `_mcu_id`).
- Topic and node configuration via `RosNodeConfig` filled in
  `src/<variant>/ros.cpp`. Node name and namespace come from
  `CommunicationManager` based on which transport was chosen.

Effort field on `_motors/feedback` is in **Nm** when the motor has a
configured current source (sensor or back-EMF model — see "Patterns:
effort"); otherwise it's the commanded PWM duty (-1.0 to 1.0).

Sign convention follows the URDF joint axis (right-hand rule around
`<axis xyz="...">`). Per-wheel `inv_dir` flags in encoder + motor configs
calibrate physical pin polarity to match the URDF axis. Once those are
correct, position / velocity / effort all carry URDF-consistent sign.

---

## Patterns

### IMU calibration

The BNO055 fuses orientation on-chip (NDOF mode); a factory-fresh chip's
fusion output can be several degrees off on roll/pitch until its
accel/gyro/mag calibration registers are populated. There is no external
storage on either board revision (no I2C EEPROM wired to the IMU bus, no
VBAT-backed RTC domain), so calibration offsets ride in
`persistent_config`'s flash-sector-11 record alongside the comm
backend/namespace. `boards/rosbot_stm32f407.json`'s `upload.maximum_size`
(917504 = the sector 10 boundary, not the chip's full 1 MB) makes the
STM32duino core's linker script size the `FLASH` region to match, so a
build that grows past sector 10 fails at link time instead of silently
letting a reflash overwrite this record — see `persistent_config.hpp`.

`persistent_config::save()` asserts the scheduler isn't running — a
sector erase stalls 1-3 s, which would starve the motor watchdog and
MAVLink TX DMA if it happened mid-drive. That rules out committing a
calibration from a live ROS/MAVLink service call. Instead, calibration is
a **boot-time window**, entirely inside `setup()` before
`vTaskStartScheduler()`:

1. `setup()` calls `resolveBootAction()` (`lib/boot_option/`) as the very
   first thing after `boardPheripheralsInit()`, before anything else —
   including `g_comm_mgr.selectTransport()` — reads the same buttons.
   `resolveBootAction()` classifies a single press by how long it's held
   and returns one of three mutually-exclusive actions, so there's no
   coordination needed between the calibration path and the
   diagnostic-transport path; they're just two outcomes of the same
   decision:
   - no press starts within `press_detect_window_ms` (1.5 s) of entry →
     `kNone`, normal boot. This window exists because the button is only
     read here, once, at the top of `setup()` — an operator who presses
     it a few hundred ms after the reset edge (instead of holding through
     it) would otherwise be missed entirely.
   - pressed within that window, then released before the hold threshold
     (3 s) → `kChangeTransport`. Green LED(s) latch on solid immediately
     as confirmation.
   - held past the threshold → `kCalibrateImu`, decided the instant the
     threshold is crossed (doesn't wait for release). Green LED(s) blink
     3x to confirm entry, then turn off.

   rosbot passes both push buttons as interchangeable (either one
   qualifies); rosbot_xl passes only `PUSH_BUTTON1` (`PUSH_BUTTON2` there
   is wired to MCU `NRST`, not a readable GPIO). Boards with two green
   LEDs (rosbot) light both together for every confirmation, so the two
   robots read identically with one LED's worth of vocabulary.

   `kChangeTransport` is wired to `g_comm_mgr`'s `useDiagnosticCondition`
   callback (now just returns the precomputed bool — no more live GPIO
   polling from inside that callback, and no `onDiagnosticSelected`
   callback either, since BootOption already lit the LED).
2. On `kCalibrateImu`, firmware polls `ImuBno055::getCalibrationStatus()`
   in a blocking loop (RED_LED blinking, via `imu_calibration_boot::run()`
   in `lib/imu/imu_calibration_boot.*` — shared by both variants) while
   the operator moves the robot — gyro settles by sitting still, mag by a
   figure-8 rotation, accel needs a few stable rests >45° apart, which is
   awkward on an assembled wheeled robot; a fixture/stand is worth having
   on a production line rather than relying on freehand tilting.
3. On `sys/gyro/accel/mag == 3/3/3/3` (or a 120 s timeout), GRN_LED(s) go
   solid, offsets are captured via `captureCalibrationOffsets()` and
   folded into the same `persistent_config::Config` that's about to be
   saved for comm backend/namespace — one erase+program cycle, not two.
4. Every boot, if a calibration record is present,
   `ImuBno055::applyCalibrationOffsets()` loads it into the chip right
   after `init()`, so fusion starts pre-calibrated instead of drifting in
   from scratch.

Calibrating and switching to the diagnostic transport in the same boot
isn't supported — the two are separate actions on the same gesture axis,
not combinable. Wanting both means two boots (either order): both settle
into the same persisted `Config`, so nothing is lost between them.

There's deliberately no ROS/MAVLink service exposing calibration status
live — `getCalibrationStatus()` only works via blocking Wire calls before
`enableDmaReads()` (see the gotcha below), and no link exists yet at that
point in boot anyway (ROS/MAVLink both come up well after this window,
and after `enableDmaReads()`). A runtime service calling it would just
fail. Progress is observable only via the diagnostic-serial logs and LEDs
during the boot-time window itself — see `imu_calibration_boot::run()`.

Step 2's blocking calls (`getCalibrationStatus()`,
`captureCalibrationOffsets()`) only work because `main.cpp` calls
`ImuBno055::init()` without following it with `enableDmaReads()` until
*after* this whole window — see the "FreeRTOS-safe IRQ priorities"
gotcha above for why that ordering matters.

### Hi-Z motor control

`MotorHiZ` drives any dual-IN H-bridge whose truth table is:

| IN1 | IN2 | output |
|---|---|---|
| H | L | forward |
| L | H | reverse |
| H | H | brake (low-side short) |
| L | L | coast (Hi-Z) |

The trick: PWM is generated by a hardware timer on `pwm_pin`, but it's
not connected to a separate enable pin. Instead, one of the IN pins is
set to `INPUT` (Hi-Z), letting its alternate-function PWM drive the line.
The other IN pin is `OUTPUT LOW` to set direction.

Compatible with TI DRV8848, TI DRV8870, Analog MAX22205. Adding another
chip with the same truth table needs no code change — just a config
struct.

### Effort source dispatch

`MotorHiZ::applyPWM(duty)` picks one of two paths each cycle to populate
`current_effort_`:

- **Sensor path** (`sampleCurrent`) — when
  `cfg.current_sense_pin != 0xFF` and not runtime-disabled. Reads ADC,
  scales by `cfg.current_per_volt`, signs by `current_mode_`, EMA-filters,
  multiplies by `cfg.torque_constant`. Used on rosbot_xl rev 1.2 with
  MAX22205 CSO.
- **Estimator path** (`estimateCurrent`) — when sensor not available.
  Computes `I = (duty·V_supply − Ke·ω_motor) / R` from the steady-state
  DC motor model. `V_supply` comes from a free-function pointer set via
  `setSupplyVoltageProvider` (typically reads
  `g_battery->getData().voltage`). EMA-filters, multiplies by
  `torque_constant`. Used on rosbot, and on rosbot_xl rev 1.1 (after
  `disableCurrentSensor()` is called for that revision).

Both paths feed into the same `applyCurrentSample` helper that handles
EMA + torque scaling.

Motor parameter derivation methodology — given a gear-motor data sheet
(no-load RPM at output, no-load current, stall torque at output, gear
ratio N), solve simultaneously:

```
(1) no-load:   V = Ke·ω_motor + I_no_load·R
(2) stall:     V = I_stall·R              (back-EMF = 0)
(3) stall τ:   τ_stall_output = Ke·I_stall·N·η     (Kt_motor = Ke in SI)

Closed form, given assumed η ≈ 0.75:
    I_stall  = τ_stall·ω_no_load_motor/(N·η·V) + I_no_load
    R        = V / I_stall
    Ke       = τ_stall / (N·η·I_stall)
    Kt_total = Ke·N·η
```

`ω_no_load_motor = ω_no_load_output × N` (gearbox un-reduction).

### Motor owns its encoder

`MotorHiZ::init()` calls `encoder_->init()`; `MotorHiZ::update()` calls
`encoder_->update()` before the PID step. There is no separate
`g_encoders.update()` pass in the control task. Ownership is explicit:
the motor pointer holds the encoder pointer; their lifetimes are
co-managed.

### micro-ROS transport (event-driven)

The default Arduino transports (`arduino_native_ethernet_udp_transport_*`,
`Stream::readBytes`) busy-poll, keeping the uRos task in the Running
state and burning CPU. The transports in `lib/ros/ros/transport/` replace
both with blocking primitives:

- **`lwip_udp_transport`** (rosbot_xl) — bypasses Arduino `EthernetUDP`,
  uses LwIP raw API directly. `udp_recv()` callback (in LwIP scheduler
  thread) pushes incoming UDP payload to a FreeRTOS stream buffer.
  `read()` blocks on `xStreamBufferReceive(timeout)`. `write()` calls
  `udp_sendto()` (LwIP TX is already DMA-driven). Local bind port =
  agent port (matches the prior Arduino convention so the agent setup
  stays the same).
- **`serial_transport`** (rosbot) —
  - **RX**: replaces `Stream::readBytes`'s busy-poll with an
    `available()`-based loop that calls `vTaskDelay(1)` when the ring
    buffer is empty. Uses `vTaskSetTimeOutState` /
    `xTaskCheckForTimeOut` for tick-wraparound-safe timing. **Not fully
    event-driven** — `HardwareSerial::_serial` is private in the
    framework and `HAL_UART_RxCpltCallback` is a strong symbol, so we
    cannot register a per-byte semaphore signal without patching the
    framework. The yielding poll buys most of the win at zero invasion.
  - **TX**: DMA-driven. `write()` pushes bytes into a 2 KB
    `xStreamBuffer`; the DMA TC IRQ chains the next 256 B chunk
    autonomously and only marks idle when the buffer drains. Replaces
    the per-byte TX IRQ that previously dominated uRos CPU. Backpressure:
    `xStreamBufferSend` blocks the caller for up to 5 ms when the buffer
    is full, then returns the partial count (silent drops would corrupt
    xrce-dds framing). DMA stream + channel resolved at runtime from the
    Serial pointer — see "USART → DMA mapping" below.

Measured impact: rosbot_xl `uRos` 69 % → 8 %. rosbot `uRos` 32 % → 7 %.

`SPIN_TIME_MS` in `RosNodeConfig` is the timeout passed to
`rclc_executor_spin_some`. With event-driven transports, this is "max
time the task will be blocked waiting for data" — 50 ms on rosbot_xl,
10 ms on rosbot are reasonable defaults. The 10 ms `TIMER_MS` ensures
`rcl_wait` returns at least every 10 ms regardless to fire publishers.

`RosNode::ethernetTransportInit` / `serialTransportInit` register the
transport's four callbacks via `rmw_uros_set_custom_transport`.

### FreeRTOS-safe IRQ priorities

`configMAX_SYSCALL_INTERRUPT_PRIORITY = 5` (numerical). Cortex-M
convention: lower number = higher priority. Any IRQ that calls
`*FromISR()` API must run at priority **≥ 5** numerically.

The framework defaults I2C IRQs to priority 2 (above
`configMAX_SYSCALL_INTERRUPT_PRIORITY`), which would crash if our
override of `HAL_I2C_MemRxCpltCallback` ran a `*FromISR` call. The IMU
DMA path lowers the EV/ER + DMA stream IRQ priority to 5 — canonical
pattern in `lib/imu/imu_bno055.cpp`. Replicate this if you add another
HAL-callback-driven path on top of a framework-managed peripheral.

**Gotcha (HW-verified 2026-08-26):** that priority change — and linking
our DMA handle into `s_hi2c` via `__HAL_LINKDMA` — breaks Wire's own
blocking transactions (`HAL_I2C_Master_Receive_IT`, what
`Adafruit_BNO055`'s non-DMA calls use). Confirmed by probing the I2C bus
directly before/after: reads succeed before, fail with a generic HAL
error immediately after. Boot-time IMU calibration (see "IMU
calibration" below) needs those blocking calls, so `ImuBno055::init()`
only brings the chip up (NDOF mode, axis remap) — the DMA/IRQ setup is a
separate `enableDmaReads()`, called once right before
`vTaskStartScheduler()`, after calibration is done. `update()` no-ops
safely (`s_done_sem == nullptr` guard) until then, and no task calls it
before the scheduler starts anyway.

### DMA + FreeRTOS handshake

Standard pattern, reused for IMU and (planned) UART TX. Steps:

1. `__HAL_RCC_DMAx_CLK_ENABLE()`.
2. Configure `DMA_HandleTypeDef` (direction, increments, sizes).
3. `HAL_DMA_Init`.
4. `__HAL_LINKDMA(periph_handle, hdmarx/hdmatx, our_hdma)`.
5. `HAL_NVIC_SetPriority(stream_irqn, 5, 0)` and enable.
6. Create a binary semaphore (or use `StreamBuffer`).
7. In task: call `HAL_..._DMA(...)` then
   `xSemaphoreTake(timeout)`.
8. Define `extern "C" void DMAx_StreamY_IRQHandler() {
     HAL_DMA_IRQHandler(&our_hdma); }` (the framework leaves DMA stream
   IRQ vectors weak by default).
9. Override `HAL_..._CpltCallback` (peripheral-specific) to do
   `xSemaphoreGiveFromISR + portYIELD_FROM_ISR`.

Caveat: some HAL completion callbacks are strong-symbol in the framework
(e.g. `HAL_I2C_ErrorCallback` in `Wire/utility/twi.c`). When that
happens, fall back to the timeout in `xSemaphoreTake` and abort the
transfer manually (`HAL_I2C_Master_Abort_IT` for I2C).

### Wire 100 kHz reset gotcha

`TwoWire::begin()` in stm32duino unconditionally calls
`i2c_init(&_i2c, 100000, ...)` — hard-coded 100 kHz. Any third-party
library that does its own `Wire.begin()` (e.g. Adafruit_BNO055 inside
its `begin(mode)`) silently drops your previously-configured 400 kHz
back to 100 kHz.

Workaround: re-apply `bus->setClock(400000)` in your driver's `init()`
**after** the third-party library finishes setup. Note that this didn't
turn out to be the dominant cost on this hardware — the BNO055 also has
its own clock-stretch behavior — but it's a real footgun.

### USART → DMA mapping (and IRQ-handler symbol collisions)

`serial_transport` resolves the DMA stream / channel for each Serial at
runtime via `findTxMap(HardwareSerial*)`. Currently mapped: `&Serial1`
(USART1) and `&Serial3` (USART3). To extend to another Serial, add an
entry in the function plus an IRQ handler symbol — but check the table
below first for stream-IRQ collisions, since `lib/` is shared across
variants and IRQ handlers are link-time strong symbols.

STM32F4 USART/UART TX → DMA mapping (RM0090 Table 43):

| Serial | UART | TX DMA primary | TX DMA alt |
|---|---|---|---|
| Serial1 | USART1 | DMA2 Stream 7 Ch4 | — |
| Serial2 | USART2 | DMA1 Stream 6 Ch4 | — |
| Serial3 | USART3 | DMA1 Stream 3 Ch4 | DMA1 Stream 4 Ch7 |
| Serial4 | UART4 | DMA1 Stream 4 Ch4 | — |
| Serial5 | UART5 | DMA1 Stream 7 Ch4 | — |
| Serial6 | USART6 | DMA2 Stream 6 Ch5 | DMA2 Stream 7 Ch5 |
| Serial7 | UART7 | DMA1 Stream 1 Ch5 | — |
| Serial8 | UART8 | DMA1 Stream 0 Ch5 | — |

Already-defined `DMAx_StreamY_IRQHandler` symbols in `lib/`:

| Symbol | Defined by | Reason |
|---|---|---|
| `DMA1_Stream0_IRQHandler` | `imu_bno055.cpp` | I2C1_RX (placeholder, not used today) |
| `DMA1_Stream2_IRQHandler` | `imu_bno055.cpp` | I2C3_RX — rosbot IMU |
| `DMA1_Stream3_IRQHandler` | `imu_bno055.cpp` | I2C2_RX — rosbot_xl IMU |
| `DMA1_Stream4_IRQHandler` | `serial_transport.cpp` | USART3_TX (alt mapping) |
| `DMA2_Stream7_IRQHandler` | `serial_transport.cpp` | USART1_TX |

Picking the alt mapping for USART3_TX (Stream 4 Ch7 instead of the
primary Stream 3 Ch4) was deliberate: the primary collides with
`imu_bno055.cpp`'s `DMA1_Stream3_IRQHandler` symbol, which is in the
link on both variants even though only rosbot_xl uses it.

Recipe for adding a new Serial to `serial_transport`:

1. Pick a stream (primary or alt) that does not collide with any symbol
   in the table above.
2. Add a `findTxMap` entry guarded by `defined(USARTx_BASE) &&
   defined(ENABLE_HWSERIALx)`:
   ```cpp
   if (serial == &SerialN) {
     static const UartTxDmaMap kMap = {USARTN, DMAx, DMAx_StreamY,
                                       DMA_CHANNEL_z, DMAx_StreamY_IRQn};
     return &kMap;
   }
   ```
3. Add the matching IRQ handler at namespace scope:
   ```cpp
   extern "C" void DMAx_StreamY_IRQHandler(void) {
     if (s_hdma_tx.Instance == DMAx_StreamY) HAL_DMA_IRQHandler(&s_hdma_tx);
   }
   ```
4. Add `-D ENABLE_HWSERIALx` to the relevant `[env:...]` in
   `platformio.ini` if the framework hasn't enabled it already.
5. Add the new symbol to the "already-defined" table in this section so
   the next person picking a stream sees it.

If you ever need 4+ DMA-driven peripherals on shared streams, consider
refactoring the IRQ handlers into a central dispatcher (`dma_dispatch.cpp`)
where each module registers its own callback; current code keeps it
simple because the conflict surface is small.

### Variant universality in `lib/`

Library code must not assume which variant compiled it. Patterns that
help:

- Look up peripheral-specific tables at runtime from the I2C / UART /
  Timer instance pointer (e.g. `findRxMap(I2C_TypeDef*)` in IMU). One
  binary works on both, controlled by config struct values.
- Wrap optional features in null-pointer / sentinel checks
  (`current_sense_pin == 0xFF` disables the analog current sensor; same
  pattern for `back_emf_constant <= 0` disabling the estimator).
- Provide runtime-disable setters (`disableCurrentSensor()`) for cases
  where the same config is shared across revisions but the peripheral
  isn't present.

---

## Build

`platformio.ini` defines a base `[env]` with shared
`framework-arduinoststm32` (Husarion fork), STM32Ethernet, LwIP,
micro_ros_arduino, STM32FreeRTOS, Adafruit BNO055, VL53L0X. Then four
concrete envs select variant + debug/release:

`board = rosbot_stm32f407` is a repo-local definition in `boards/`
(STM32F407ZGT6, generic `variant_generic.h` exposing all GPIO). It is the
honestly-named successor to the misleading `rosbot_xl_digital_board` — both
resolve to the identical generic F407ZG build (`ARDUINO_GENERIC_F407ZGTX`),
shared by **both** variants. The Husarion `framework-arduinoststm32` fork is
still required: it bumps the serial RX/TX buffers (64→512) and splits the
Ethernet pin map into `PinMap_Ethernet_MII/RMII` so RMII mode only claims its
9 pins (upstream's single `PinMap_Ethernet[]` would grab MII-only GPIO used
elsewhere on rosbot_xl).

- `[env:rosbot]` — debug, `-D ROSBOT`, `-D ENABLE_HWSERIAL1`,
  `-D ENABLE_HWSERIAL3`, `build_src_filter = +<rosbot/*> -<rosbot_xl/*>`.
- `[env:rosbot_release]` — same + `[release_flags]` (`-O2 -D RELEASE`,
  cortex-m4 / fpv4 flags). Strips `-g`.
- `[env:rosbot_xl]` — debug, `-D ROSBOT_XL`, `-D ENABLE_HWSERIAL1`,
  `-D ETHERNET_USE_FREERTOS`, `-D LAN9303`,
  `build_src_filter = -<rosbot/*> +<rosbot_xl/*>`.
- `[env:rosbot_xl_release]` — release variant of the above.

There are no separate `_mavlink` envs anymore. Both upstream-link
backends (micro-ROS and MAVLink) live in the same binary; the choice
happens at boot via the BACKEND: handshake line — see the MAVLink build
section below for the runtime-switch mechanism.

`-D FW_VERSION=\"vX.Y.Z-jazzy\"` carries the firmware version. The release
workflow (`.github/workflows/release.yaml`) bumps it before tagging.

Build output sizes (release, both backends linked):
- rosbot ~200 KB Flash (19 %), ~54 KB RAM (41 %)
- rosbot_xl ~238 KB Flash (23 %), ~96 KB RAM (74 %)

The single-binary variants weigh ~12 KB more than the previous
micro-ROS-only release builds — the MAVLink stack adds publishers,
subscribers, the dialect-encoder fast paths and the `MavlinkNode`
itself on top of what the micro-ROS path already linked. With both
stacks in one image RAM grew ~2 KB (the second link's globals;
neither stack's queues are doubled because the producer tasks feed
both via the same `lib/mavlink/mavlink_types.hpp` queue wrappers).

CCM RAM usage is implicit (compiler may place stack/BSS there). DMA
buffers are explicitly declared with `alignas(4)` at file scope to land
in regular SRAM.

---

## MAVLink build

The MAVLink stack lives in the same binary as the micro-ROS stack; the
firmware picks between them at boot. The wire protocol on the MCU↔SBC
link is **MAVLink v2** with a custom `rosbot` dialect when MAVLink is
selected; an in-tree ROS 2 bridge (`bridge/rosbot_mavlink_bridge/`)
re-exposes the same node name, topic names, types and QoS that the
existing micro-ROS firmware advertises today. From a downstream consumer
(`rosbot_ros`) point of view the wire protocol switch is invisible.

Full design and rollout plan: [MAVLINK_MIGRATION.md](./MAVLINK_MIGRATION.md).

### Runtime backend dispatch

`CommunicationManager::waitForHostConfig` accepts three line types in
any order during the boot-time handshake window (~2.5 s after MCU
reset): `BACKEND:microros|mavlink`, `NS:<namespace>`, and `END`. `END`
(or the timeout) closes the handshake; the others are independently
optional. The chosen backend drives the upstream-link selection in
`setup()`:

```cpp
if (g_comm_mgr.getSelectedBackend() == CommBackend::MAVLINK) {
  g_mavlink_node.setNamespace(...); g_mavlink_node.begin();
  g_link = &g_mavlink_node;
} else {
  g_ros_node.setNamespace(...); g_ros_node.serialTransportInit(...);
  g_link = &g_ros_node;
}
```

Both `g_ros_node` and `g_mavlink_node` are constructed as globals
(their constructors only store config — no HW touched), so they
coexist in `.bss` without contention. Only the chosen one's `init()`/
`begin()` is called, so only one drives peripherals. `g_link` is a
pointer assigned in `setup()` before `vTaskStartScheduler()`, so any
task body sees a non-null pointer when it first dereferences.

Default backend on handshake timeout is `MICRO_ROS` — that preserves
the legacy behaviour for older host drivers that don't emit the
`BACKEND:` line. The `rosbot_ros/configure_robot` script in the
matching `feat/runtime-comm-impl` branch passes `--backend
microros|mavlink` based on which launch file (microros.launch.py /
mavlink.launch.py) ran.

### Layout

- `lib/mavlink/`
  - `mavlink_node.{hpp,cpp}` — state machine + HEARTBEAT/TIMESYNC/STATUSTEXT.
    Equivalent of `RosNode` for the MAVLink path; inherits `RoboticsLink`
    so the `uRos` task can drive either build via the same `g_link`
    reference.
  - `publishers/{battery,imu,joint_state,buttons,range}_publisher.hpp` —
    pull from FreeRTOS queues, pack the corresponding `mavlink_message_t`,
    forward via `MavlinkNode::sendMessage()`.
  - `subscribers/{wheel_cmd,led,led_strip}_subscriber.hpp` and
    `commands/mcu_id_command.hpp` — dispatched by msgid from the rx loop.
  - `transport/mavlink_{serial,udp}_transport.{hpp,cpp}` — same DMA-TX +
    yielding-poll RX (serial) / LwIP raw API (UDP) patterns as
    `lib/ros/ros/transport/`, just stripped of XRCE framing.
  - `dialect/rosbot.xml` — dialect source of truth. The mavgen C output
    lives inside the bridge package at
    `bridge/rosbot_mavlink_bridge/mavlink_dialect/` (single canonical
    location); the firmware reaches it via the include path in
    `platformio.ini`.
- `include/robotics_link.hpp` — abstract base both `RosNode` and
  `MavlinkNode` inherit; `src/<variant>/rtos.cpp` calls `g_link->loop()` /
  `g_link->isConnected()` uniformly. `g_link` is `RoboticsLink*` now —
  `main.cpp` assigns it to whichever singleton the boot handshake picked.
- `src/<variant>/main.cpp` — variant entry point. Both backends linked.
- `src/<variant>/mavlink_entities.cpp` — MAVLink publisher/subscriber
  registration + the `g_mavlink_node` definition. Always compiled; its
  `begin()` only runs when the backend dispatch picks MAVLink. (Renamed
  from `ros_mavlink.cpp` so the filename matches its role — the file no
  longer contains a separate `main_*`.)

### Topology

- **rosbot**: SBC ↔ MCU over Serial1 @ 921600. Mirrors the micro-ROS
  serial transport file-for-file, sans XRCE.
- **rosbot_xl**: SBC ↔ MCU over UDP. MCU binds **14555**, sends to peer
  at **14550** (mavros default port layout, D17). Bridge does the
  opposite — binds 14550, sends to 14555 on the MCU IP.

### State machine

`MavlinkNode::loop()` cycles WAITING → AWAIT_TIMESYNC → CONNECTED →
DISCONNECTED:

```
WAITING        send HEARTBEAT 1 Hz, retry boot STATUSTEXT every 1 s for
                up to 10 s; on first peer HEARTBEAT → AWAIT_TIMESYNC
AWAIT_TIMESYNC send TIMESYNC every 200 ms; on first reply → CONNECTED
CONNECTED      send HEARTBEAT 1 Hz, TIMESYNC 0.5 Hz, telemetry per §4
                rates; if no peer HEARTBEAT for 3 s → DISCONNECTED
DISCONNECTED   reset → WAITING (motor watchdog already stopped wheels
                500 ms after the last command per D12)
```

### Telemetry rates and topic mapping

| ROS topic | Wire | Rate | Stamping |
|---|---|---|---|
| `battery` | `BATTERY_STATUS` (147) | 1 Hz | bridge wall clock |
| `_imu/data` | `ROSBOT_IMU` (11001) | 100 Hz | MCU `time_boot_us` + TIMESYNC offset |
| `_motors/feedback` | `ROSBOT_JOINT_STATE` (11002) | 200 Hz | same |
| `ranges` (rosbot) | `DISTANCE_SENSOR` (132) × 4 | 10 Hz each | `time_boot_ms` + offset |
| `buttons` | `ROSBOT_BUTTONS` (11003) | 20 Hz | same |
| `_motors/cmd` | `ROSBOT_WHEEL_SETPOINTS` (11010) | on-demand | bridge wall clock |
| `leds` | `ROSBOT_PANEL_LEDS` (11011) | on-demand | — |
| `led_strip` (rosbot_xl) | `ROSBOT_LED_STRIP` (11012) | on-demand | — |
| `_mcu_id` (service) | `COMMAND_LONG(MAV_CMD_USER_1)` → `ROSBOT_MCU_ID` (11020) | — | — |

### API parity vs micro-ROS

`ros2 node info /<ns>/rosbot_mcu` and `ros2 topic info -v /<ns>/<topic>`
against the bridge produce the same topic list, node name, types and QoS
as `micro_ros_agent` running today. The bridge's rclcpp Node additionally
advertises the standard parameter services and `/rosout` publisher (the
rcl-based micro-ROS firmware does not). These are additive and do not
affect downstream consumers. Topic type hashes are `RIHS01_*` (valid) on
the bridge vs `INVALID` on micro-ROS — also additive.

### Sizes (single-binary release)

| variant | Flash | RAM | headroom |
|---|---:|---:|---:|
| `rosbot_release` | 200 KB / 1024 KB (19 %) | 54 KB / 128 KB (41 %) | ~80 % |
| `rosbot_xl_release` | 238 KB / 1024 KB (23 %) | 96 KB / 128 KB (74 %) | ~77 % |

For reference, the previous separate builds measured:
- rosbot (micro-ROS only): 188 KB Flash / 52 KB RAM
- rosbot_mavlink (MAVLink only): 82 KB Flash / 12 KB RAM
- rosbot_xl (micro-ROS only): 226 KB Flash / 94 KB RAM
- rosbot_xl_mavlink (MAVLink only): 119 KB Flash / 55 KB RAM

The single-binary merge cost is ~12 KB Flash + ~2 KB RAM per variant —
much less than the naive `µROS_only + MAVLink_only` upper bound because
the Arduino core, FreeRTOS, motor/encoder/IMU stacks are linked once.

### Bridge package

[`bridge/rosbot_mavlink_bridge`](./bridge/rosbot_mavlink_bridge) — single
`ament_cmake` package built for both jazzy and humble out of one source
tree (D24). The dialect headers live inside the package at
[`bridge/rosbot_mavlink_bridge/mavlink_dialect/`](./bridge/rosbot_mavlink_bridge/mavlink_dialect/)
— this is the **canonical** mavgen output location, not a mirror. The
firmware build reads from the same directory via its include path
(`-I bridge/rosbot_mavlink_bridge/mavlink_dialect/rosbot` in
`platformio.ini`), so there is no duplication. Keeping the headers
inside the package makes the bridge self-contained for bloom releases
to rosdistro (the source tarball archives only the package subtree).
Launch files take a `namespace` arg and set `--ros-args -r __ns:=<value>`
on the node, so rclcpp prefixes every relative topic / service with the
same namespace the micro-ROS firmware negotiates over FTDI.

---

## Open work / known limitations

These are documented to avoid re-discovery:

- **uRos RX path on rosbot is still polling.** TX is now DMA-driven, but
  RX uses a yielding `vTaskDelay(1)` poll because `HardwareSerial::_serial`
  is private in the framework and `HAL_UART_RxCpltCallback` is a strong
  symbol — neither lets us register a per-byte semaphore signal without
  patching the stm32duino fork (or replacing `USARTx_IRQHandler`, also a
  strong symbol). The current poll buys most of the win at zero invasion
  but leaves uRos with a CPU floor proportional to read activity.
- **IMU publish rate on rosbot was ~85 Hz** (not the 100 Hz queued by the
  IMU task) due to uRos timer-callback jitter — when uRos couldn't meet
  the 10 ms tick, samples in the depth-1 queue got coalesced before
  publish. Should improve after DMA TX landed; pending fresh measurement.
- **No agent IP auto-discovery.** `AGENT_IP` is hardcoded in
  `include/rosbot_xl/config.hpp`. `CLIENT_IP` is auto-derived from it
  (same /24, last octet = agent + 1). True auto-discovery options were
  considered (DHCP server on MCU, broadcast announcement protocol, mDNS)
  but not implemented yet — see commit / chat history for trade-offs.
- **Race on shared I2C bus** (rosbot_xl `i2c`): IMU DMA path and Wire
  access from EEPROM init touch the same peripheral. EEPROM is only used
  during boot setup so the race window is closed before tasks start. No
  mutex needed today; if a future feature uses I2C2 from a task, add a
  bus mutex.
- **`HAL_I2C_ErrorCallback` is a strong symbol** in the framework, so
  the IMU DMA path cannot signal a "DMA failed" semaphore — relies on
  the read timeout instead. Acceptable; documented in
  `imu_bno055.cpp`.

When closing one of these items, remove the bullet here and add a
matching entry to the relevant "Patterns" section.
