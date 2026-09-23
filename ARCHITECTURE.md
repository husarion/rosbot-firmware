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
5. **Project libraries** in `lib/`, including the MAVLink stack
   (`lib/mavlink/`).
6. **Variant entry** in `src/<variant>/`.

micro-ROS (`micro_ros_arduino`) used to sit here as a second, boot-selected
upstream stack; it was removed after v2.1.0-jazzy (see git history).

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
| `persistent_config/` | Namespace + BNO055 calibration offsets, stored as one record appended to a log in flash sector 11. Appending is safe at runtime; only the 1-3 s sector erase (sector full) is deferred to a pre-scheduler `save()` — see "IMU calibration" |
| `pid/` | PID controller with feedforward, anti-windup, dead-zone boost |
| `power_board/` | UART protocol to rosbot_xl power board MCU (battery state) |
| `range/` | `RangeInterface` + `RangeVl53l0x` + `RangeArray` (rosbot only) |
| `imu_calibration/` | Runtime calibration: session flag (LED), save request serviced from `imuTask` — see "IMU calibration" |
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
| Link | COMMUNICATION | 200 | `g_mavlink_node.loop()` — MAVLink RX/TX, heartbeat, publishers |

### rosbot_xl tasks

| name | priority | freq [Hz] | role |
|---|---|---|---|
| HwMonitor | OBSERVING | 10 | Battery (via PowerBoard UART) + fan + LED status, rate-limited internally |
| Imu | SENSORS | 100 | same as rosbot |
| LedStrip | COMMUNICATION | 30 | Drain `led_strip_queue`, render via SPI |
| Monitor | BLOCKING | 1 | Runtime stats (debug builds) |
| MotorControl | CONTROL | 200 | same as rosbot |
| Shutdown | OBSERVING | 3 | Detect graceful shutdown signal from power board, stop scheduler |
| Link | COMMUNICATION | 1000 | same as rosbot |

`Link` runs at 1000 Hz on rosbot_xl, 200 Hz on rosbot (the serial
transport is event-driven — see "Patterns: serial transport").

Queue depths are 1 (`xQueueOverwrite`) for telemetry — newest sample
wins, no buffering. Watchdog on `MotorArray` stops motors after 500 ms
without `setVelocities()` (in `feedWatchdog()`).

---

## ROS interface

User-facing contract is in [ROS_API.md](ROS_API.md). Implementation map:

- The ROS side lives on the SBC, in `bridge/rosbot_mavlink_bridge`.
- MCU-side publishers / subscribers / commands in `lib/mavlink/`, wired per
  variant in `src/<variant>/mavlink_entities.cpp`. The namespace comes from
  the `NS:` handshake line (`CommunicationManager`).

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
`persistent_config`'s flash-sector-11 record alongside the namespace. `boards/rosbot_stm32f407.json`'s `upload.maximum_size`
(917504 = the sector 10 boundary, not the chip's full 1 MB) makes the
STM32duino core's linker script size the `FLASH` region to match, so a
build that grows past sector 10 fails at link time instead of silently
letting a reflash overwrite this record — see `persistent_config.hpp`.

A sector erase stalls the CPU for 1-3 s, which would starve the motor
watchdog and MAVLink TX DMA if it happened mid-drive. So the sector is an
append-only log of fixed-size records: `load()` takes the newest valid
one, and `save()` only *programs* the next free slot (~70 bytes, no
stall). The erase happens only when the sector is full (~1900 records),
and then only from a `save()` before the scheduler starts — at runtime it
returns false instead.

There are two ways to calibrate. Both end in the same record.

**Runtime (the normal path).** The BNO055 calibrates
continuously in NDOF, so nothing needs a reboot: the host only watches and
saves. `ROSBOT_IMU_CALIBRATION` (5 Hz) carries CALIB_STAT plus the save
state; `COMMAND_LONG` `MAV_CMD_USER_2` with `param1` = 1 start / 0 stop /
2 save. The bridge exposes them as `_imu/calibration` and
`_imu/{start,stop,save}_calibration` — see [ROS_API.md](ROS_API.md).

- Start/stop only drive the LED: the red LED blinks at 100 ms while a
  session is on (180 s cap) and goes dark once gyro/accel/mag reach 3.
- Save is refused in the command handler when the chip is not calibrated,
  otherwise it sets a flag that `imuTask` services before its next
  `update()`, so the I2C bus has one owner. Reading the offsets needs
  CONFIG mode (~25 ms in, ~20 ms back to NDOF); measured IMU gap during a
  save ~60 ms.
- The criterion is gyro/accel/mag == 3; `sys` is ignored. Per the
  datasheet those three are the offset status, `sys` is fusion confidence,
  and on ROSbot 3 it hovered at 0-2 while all three sat at 3.
- Right after boot, with offsets freshly restored, CALIB_STAT can report
  mag=3 for a moment before dropping to 0 (seen on ROSbot 3). A save then
  just rewrites the restored offsets — harmless, but a host that
  auto-saves should wait for the operator's movement, not the first 3/3/3.

**Boot-time window.** Entirely inside `setup()` before
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
3. On `gyro/accel/mag == 3/3/3` (or a 120 s timeout), GRN_LED(s) go
   solid, offsets are captured via `captureCalibrationOffsets()` and
   folded into the same `persistent_config::Config` that's about to be
   saved for the namespace — one erase+program cycle, not two.
4. Every boot, if a calibration record is present,
   `ImuBno055::applyCalibrationOffsets()` loads it into the chip right
   after `init()`, so fusion starts pre-calibrated instead of drifting in
   from scratch.

Calibrating and switching to the diagnostic transport in the same boot
isn't supported — the two are separate actions on the same gesture axis,
not combinable. Wanting both means two boots (either order): both settle
into the same persisted `Config`, so nothing is lost between them.

During the boot window no link exists yet, so progress is observable only
via the serial logs and LEDs — see `imu_calibration_boot::run()`. At
runtime the status comes for free: CALIB_STAT (0x35) ends the same DMA
block `update()` already reads (0x08..0x35, 46 bytes).

Where those logs go differs per variant, because an SBC can only read what
is wired to it:

- **ROSbot XL** — the diagnostic serial (FT230X, `/dev/rosbot` on the SBC).
  The upstream link is Ethernet, so the FTDI is free during the window.
- **ROSbot 3** — the diagnostic serial *and* the SBC link (Serial1, the Pi's
  `/dev/ttyAMA0`). The diagnostic UART is a rear-panel header with no SBC
  wiring, so without the mirror nothing on the robot could see progress.
  The link is idle then — MAVLink only starts after `run()`
  returns, the same window the pre-comm `FW:` prompt already uses — but the
  host driver keeps the port open, and two readers on one tty split the
  bytes between them rather than both seeing them. So the host side does not
  open the port a second time: `rosbot_mavlink_bridge` feeds every byte that
  is not part of a MAVLink frame to a line collector
  (`bridge/.../boot_text.hpp`) and logs `IMU calibration:` / `IMU init
  failed` lines as `[MCU boot] ...`. Those lines therefore stay plain ASCII.
  Not mirrored when the FTDI itself is the link (`kChangeTransport`, a
  developer setup): there is no free debug line then anyway.

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

### Serial / UDP transport (event-driven)

The default Arduino paths (`EthernetUDP`, `Stream::readBytes`) busy-poll,
keeping the link task in the Running state and burning CPU. The transports
in `lib/mavlink/transport/` replace both with blocking primitives:

- **`mavlink_udp_transport`** (rosbot_xl) — bypasses Arduino `EthernetUDP`,
  uses the LwIP raw API directly. The `udp_recv()` callback (LwIP thread)
  pushes the payload into a FreeRTOS stream buffer; `read()` blocks on
  `xStreamBufferReceive(timeout)`; `write()` calls `udp_sendto()` (LwIP TX
  is already DMA-driven).
- **`mavlink_serial_transport`** (rosbot) —
  - **RX**: an `available()`-based loop that calls `vTaskDelay(1)` when the
    ring buffer is empty, with `vTaskSetTimeOutState` /
    `xTaskCheckForTimeOut` for tick-wraparound-safe timing. **Not fully
    event-driven** — `HardwareSerial::_serial` is private in the framework
    and `HAL_UART_RxCpltCallback` is a strong symbol, so a per-byte
    semaphore would need a framework patch. The yielding poll buys most of
    the win at zero invasion.
  - **TX**: DMA-driven. `write()` pushes bytes into a 2 KB `xStreamBuffer`;
    the DMA TC IRQ chains the next chunk and only marks idle when the buffer
    drains. `xStreamBufferSend` blocks the caller for a few ms when the
    buffer is full, then returns the partial count. DMA stream + channel are
    resolved at runtime from the Serial pointer — see "USART → DMA mapping"
    below.

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

`ImuBno055::init()` re-applies the fast-mode clock after `bno_.begin()`.
This was documented here long before the code did it, and it mattered:
at 100 kHz the DMA read took 5-9 ms against a 4 ms timeout, nearly every
read timed out, and `imuTask` kept republishing the last good sample —
orientation frozen while the robot was turned by hand (ROSbot 3, 2026-09).
At 400 kHz: 1.6-6.9 ms, 0 timeouts in 2181 reads. The spread is the
BNO055's clock stretching plus task wake-up jitter, hence
`kReadTimeoutMs = 8` (under the 10 ms task period). `update()` now reports
whether it read a fresh sample, and `imuTask` publishes only those.

`setClock(400000)` itself is not 400 kHz either: stm32duino always picks
the 16/9 duty cycle for fast mode, and with PCLK1 = 42 MHz CCR rounds up
to 5, i.e. 336 kHz (read back from `I2C->CCR`). Use `setI2cFastMode()`
(`lib/i2c_fast_mode/`) for every bus: it switches to the 2:1 duty cycle,
which divides evenly (CCR 35 = exactly 400 kHz) and stays inside the
fast-mode tLOW/tHIGH minimums.

### Timing measured on HW (2026-09)

Every task was instrumented (DWT cycle counter, wake-to-wake period and
execution time, per-task CPU from the FreeRTOS run-time stats) on
`rosbot_release` / `rosbot_xl_release`. All tasks hit their configured
rate; idle is ~92% on ROSbot 3 and ~89% on ROSbot XL. Things that did not
look right, and what changed:

- **Queue-fed MAVLink publishers dropped samples.** `publish()` moved
  `last_pub_ms_` before it knew the queue held anything, so a call that
  came a moment early closed the gate for a whole period and the next
  sample was overwritten in its depth-1 queue. With the 5 ms link loop of
  ROSbot 3, `_imu/data` arrived at 82 Hz instead of 100 (XL's 1 ms loop hid
  it). The gate now moves only on a real publish, and IMU / joint state /
  ranges use `period_ms = 0`: the producing task already sets the rate.
- **`PowerBoard::update()` spun for 100 ms.** It called `readBytes()`,
  which busy-waits for the stream timeout after the last byte, so
  `hwMonitorTask` ran 119 ms once a second. It now drains what the UART
  holds and keeps a partial frame; the reply is picked up on a later
  10 Hz tick. The board-info request goes half a second after the battery
  one, because both replies together (102 B) overflow the 64 B RX buffer.
- `rangeTask` and `ledStripTask` used `vTaskDelay`, so their period grew by
  the execution time (range: 101 ms instead of 100).
- The run-time stats timer ticks at 50 kHz, not 100 kHz: TIM5 is on APB1
  (84 MHz timer clock).

Checked and fine: motor PWM 20 kHz on all four timers, XL fan PWM 25 kHz,
encoder timers in encoder mode, USART1 923 kbaud for a 921.6 kbaud link
(+0.16%). The rear-panel USART3 on ROSbot 3 runs +1.27% fast — the best
BRR gets at 42 MHz, still inside UART tolerance. The XL LED-strip SPI
asks for 4 MHz and gets 2.625 MHz (the next power-of-two divider).

### USART → DMA mapping (and IRQ-handler symbol collisions)

`mavlink_serial_transport` resolves the DMA stream / channel for each Serial at
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
| `DMA1_Stream4_IRQHandler` | `mavlink_serial_transport.cpp` | USART3_TX (alt mapping) |
| `DMA2_Stream7_IRQHandler` | `mavlink_serial_transport.cpp` | USART1_TX |

Picking the alt mapping for USART3_TX (Stream 4 Ch7 instead of the
primary Stream 3 Ch4) was deliberate: the primary collides with
`imu_bno055.cpp`'s `DMA1_Stream3_IRQHandler` symbol, which is in the
link on both variants even though only rosbot_xl uses it.

Recipe for adding a new Serial to `mavlink_serial_transport`:

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
STM32FreeRTOS, Adafruit BNO055, VL53L0X. Then four
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

`-D FW_VERSION=\"vX.Y.Z-jazzy\"` carries the firmware version. The release
workflow (`.github/workflows/release.yaml`) bumps it before tagging.

Build output sizes (release, after micro-ROS was removed):
- rosbot ~88 KB Flash (10 %), ~11 KB static RAM (8 %)
- rosbot_xl ~125 KB Flash (14 %), ~54 KB static RAM (41 %)

With micro-ROS still linked (v2.1.0-jazzy) the same builds were 207 KB /
53 KB and 244 KB / 95 KB.

CCM RAM usage is implicit (compiler may place stack/BSS there). DMA
buffers are explicitly declared with `alignas(4)` at file scope to land
in regular SRAM.

---

## MAVLink link

The MCU↔SBC link is **MAVLink v2** with a custom `rosbot` dialect; the
in-tree ROS 2 bridge (`bridge/rosbot_mavlink_bridge/`) exposes the
`rosbot_mcu` node, topics, types and QoS that `rosbot_ros` consumes.

### Boot handshake

`CommunicationManager::waitForHostConfig` accepts three line types in any
order during the boot-time handshake window (~2.5 s after MCU reset):
`BACKEND:mavlink`, `NS:<namespace>` and `END`. `END` (or the timeout)
closes the handshake; the others are independently optional. `BACKEND:`
is a leftover of the micro-ROS era, still sent by `configure_robot`: the
firmware ACKs `mavlink` and NAKs anything else, so a host asking for
`microros` fails its handshake instead of waiting for a link that never
comes up.

### Layout

- `lib/mavlink/`
  - `mavlink_node.{hpp,cpp}` — state machine + HEARTBEAT/TIMESYNC/STATUSTEXT,
    driven by the `Link` task through `g_mavlink_node`.
  - `publishers/{battery,imu,joint_state,buttons,range}_publisher.hpp` —
    pull from FreeRTOS queues, pack the corresponding `mavlink_message_t`,
    forward via `MavlinkNode::sendMessage()`.
  - `subscribers/{wheel_cmd,led,led_strip}_subscriber.hpp` and
    `commands/mcu_id_command.hpp` — dispatched by msgid from the rx loop.
  - `transport/mavlink_{serial,udp}_transport.{hpp,cpp}` — DMA-TX +
    yielding-poll RX (serial) / LwIP raw API (UDP), see "Patterns".
  - `dialect/rosbot.xml` — dialect source of truth. The mavgen C output
    lives inside the bridge package at
    `bridge/rosbot_mavlink_bridge/mavlink_dialect/` (single canonical
    location); the firmware reaches it via the include path in
    `platformio.ini`.
- `src/<variant>/main.cpp` — variant entry point.
- `src/<variant>/mavlink_entities.cpp` — MAVLink publisher/subscriber
  registration + the `g_mavlink_node` definition.

### Topology

- **rosbot**: SBC ↔ MCU over Serial1 @ 921600.
- **rosbot_xl**: SBC ↔ MCU over UDP. MCU binds **14555**, sends to peer
  at **14550** (mavros default port layout). Bridge does the
  opposite — binds 14550, sends to 14555 on the MCU IP.

### State machine

`MavlinkNode::loop()` cycles WAITING → AWAIT_TIMESYNC → CONNECTED →
DISCONNECTED:

```
WAITING        send HEARTBEAT 1 Hz, retry boot STATUSTEXT every 1 s for
                up to 10 s; on first peer HEARTBEAT → AWAIT_TIMESYNC
AWAIT_TIMESYNC send TIMESYNC every 200 ms; on first reply → CONNECTED
CONNECTED      send HEARTBEAT 1 Hz, TIMESYNC 0.5 Hz, telemetry at the
                rates below; if no peer HEARTBEAT for 3 s → DISCONNECTED
DISCONNECTED   reset → WAITING (motor watchdog already stopped wheels
                500 ms after the last command)
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

### Bridge package

[`bridge/rosbot_mavlink_bridge`](./bridge/rosbot_mavlink_bridge) — single
`ament_cmake` package built for both jazzy and humble out of one source
tree. The dialect headers live inside the package at
[`bridge/rosbot_mavlink_bridge/mavlink_dialect/`](./bridge/rosbot_mavlink_bridge/mavlink_dialect/)
— this is the **canonical** mavgen output location, not a mirror. The
firmware build reads from the same directory via its include path
(`-I bridge/rosbot_mavlink_bridge/mavlink_dialect/rosbot` in
`platformio.ini`), so there is no duplication. Keeping the headers
inside the package makes the bridge self-contained for bloom releases
to rosdistro (the source tarball archives only the package subtree).
Launch files take a `namespace` arg and set `--ros-args -r __ns:=<value>`
on the node, so rclcpp prefixes every relative topic / service with the
namespace the host also sends the MCU in the `NS:` handshake line.

---

## Open work / known limitations

These are documented to avoid re-discovery:

- **Link RX path on rosbot is still polling.** TX is DMA-driven, but RX
  uses a yielding `vTaskDelay(1)` poll because `HardwareSerial::_serial`
  is private in the framework and `HAL_UART_RxCpltCallback` is a strong
  symbol — neither lets us register a per-byte semaphore signal without
  patching the stm32duino fork (or replacing `USARTx_IRQHandler`, also a
  strong symbol). The poll buys most of the win at zero invasion but
  leaves the link task with a CPU floor proportional to read activity.
- **No SBC IP auto-discovery.** `SBC_IP` and `CLIENT_IP` are hardcoded in
  `include/rosbot_xl/config.hpp`. True auto-discovery options were
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
