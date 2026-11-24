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
  `lib/ethernet/ethernetif.cpp` already uses for ordinary PHY register
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

## Variant model

`cmake/CMakeLists.txt` builds one executable per board, `rosbot` and
`rosbot_xl`, from the same compiled `lib/` objects (`fw_common`); each adds
its `src/<variant>/*.cpp`, and `rosbot_xl` also links `lib/ethernet`
(`fw_ethernet`). There is one build (preset `release`); the image that
goes to the robot also carries gdb symbols.

There is no variant macro: the variant
is chosen by which `src/<variant>/` is compiled, and those files include
`include/<variant>/config.hpp` directly. `lib/` never includes a variant
config and has no variant `#ifdef`; a type only one variant uses lives next
to its only user (e.g. `RangesStamped` in `publishers/range_publisher.hpp`).

Why this matters: **anything in `lib/`, `boards/` or `cmake/` affects both
variants.** Anything in `src/<variant>/` or `include/<variant>/` is
variant-only.

For runtime board-revision selection (rosbot_xl rev 1.1 vs 1.2), the MCU
reads a string from EEPROM at boot via the `BoardRevision` class. Pin
configuration that depends on revision (current-limit pins, current-sensor
enable, fan mode) is applied at runtime in `setMaxMotorsCurrent(rev)` and
`setupCurrentSense(rev)` patterns in `src/rosbot_xl/main.cpp`.

---

## Software stack

Layers, bottom-up:

1. **STM32CubeF4 HAL 1.8.1 + CMSIS device 2.6.8** (vendor, fetched by
   `cmake/dependencies.cmake`) — HAL for I2C, timers, ETH (the Legacy
   driver), flash and RCC; everything else in `lib/platform/` is on
   registers. CMSIS core headers are vendored in `third_party/cmsis_core/`.
2. **Board support** in `boards/stm32f407/` (shared by both boards):
   startup from the CMSIS template, linker script, `stm32f4xx_hal_conf.h`,
   `FreeRTOSConfig.h`, clock tree / SysTick / fault vectors / `main()` in
   `system.cpp`, the newlib heap (`heap_newlib.c`) and syscalls. No CubeMX
   project: the clock tree and every peripheral are configured in code.
3. **FreeRTOS kernel 10.3.1** — single-core preemptive scheduler, ARM_CM4F
   port, used directly (no CMSIS-RTOS). **lwIP 2.2.1** (`NO_SYS`) on
   rosbot_xl.
4. **Vendor / device libraries** — Pololu VL53L0X, vendored in
   `lib/vl53l0x/` (MIT) and moved onto `platform::I2c`; the BNO055 driver
   is our own. Plus **ETL** (Embedded Template Library, pinned tag) for
   fixed-capacity containers and `etl::span` (we build as C++17, so no
   `std::span`).
5. **Project libraries** in `lib/`, including the MAVLink stack
   (`lib/mavlink/`).
6. **Variant entry** in `src/<variant>/`.

---

## Library layer (`lib/`)

| dir | role |
|---|---|
| `battery/` | `BatteryInterface` + `BatteryAdc` (rosbot battery via ADC) + `SocEstimator` (rosbot_xl: coulomb counting corrected by load-compensated OCV with a curve measured on the XL pack, so percentage does not jump under load; rosbot: the same XL curve through a low-pass filter, as there is no current measurement and its own cells were not measured) |
| `boot_option/` | `resolveBootAction()` — classifies the power-on button gesture (tap vs 3 s hold) into "change transport" / "calibrate IMU" / nothing, owns the confirmation LEDs — see "IMU calibration" under "Patterns" |
| `comm_manager/` | `CommunicationManager` — chooses primary vs diagnostic transport at boot, driven by `boot_option`'s decision |
| `ethernet/` | rosbot_xl network bring-up: `ethernet::begin()` (static IP, lwIP task, link checks) + vendored `ethernetif` (ST/Husarion, LAN9303); `include/lwipopts.h` holds the lwIP options |
| `eeprom/` | I2C EEPROM driver + `BoardRevision` (revision string read on rosbot_xl) |
| `encoder/` | `EncoderInterface` + `HardwareEncoder` (STM32 timer in encoder mode, x4) |
| `fan/` | Fan + NTC thermistor (rosbot_xl only) |
| `fault/` | `fatalError()` and the fault/hook plumbing — see "Fatal errors" under "Patterns" |
| `imu/` | `ImuInterface` + `ImuBno055` (own BNO055 driver on `platform::I2c`: same reset/config sequence as Adafruit_BNO055 1.6.4, 46-byte data block by DMA). Also exposes on-chip calibration status/offsets, persisted via `persistent_config` — see "IMU calibration" under "Patterns" |
| `indicator/` | Status LED state machine |
| `led_strip/` | APA102-style LED strip over `platform::Spi` (rosbot_xl only) |
| `vl53l0x/` | Vendored Pololu VL53L0X driver (MIT, `LICENSE.txt`), bus calls on `platform::I2c`; excluded from pre-commit |
| `motor/` | `MotorInterface`, `MotorHiZ`, `MotorArray` (Hi-Z PWM control) |
| `persistent_config/` | Namespace + BNO055 calibration offsets, stored as one record appended to a log in flash sector 11. Appending is safe at runtime; only the 1-3 s sector erase (sector full) is deferred to a pre-scheduler `save()` — see "IMU calibration" |
| `platform/` | Hardware access on CMSIS registers / HAL. `freertos.hpp`: the kernel headers the code uses. `i2c`: HAL I2C master at 400 kHz with a per-bus mutex; once the scheduler runs every transfer uses HAL's IT API (DMA for the IMU block read) and the task sleeps until it ends; bus recovery after faults; owns the I2C IRQs and HAL I2C callbacks. `spi`: blocking SPI master. `device.hpp`: CMSIS device header + HAL. `uart`: USART1/2/3 with DMA TX/RX, see "UART driver and DMA stream map". `analog_in`: ADC1 single-shot 10-bit reads (battery, NTC, motor current), serialized with masked IRQs. `timer`: timer clock enable / kernel clock for any TIM, and `PwmOutput` (motor PWM). `clock`: TIM5 free-running at 1 MHz (also the FreeRTOS run-time stats source, see `time_base.cpp`) (`init()` is the first line of `setup()`), `delayUs()`, `delayMs()` (blocks the task once the scheduler runs). `Pin`: constexpr GPIO (`Pin{Port::E, 2}`) with output/input/analog setup under masked IRQs, used for every pure-GPIO pin in both board configs (timer and ADC pins too, via `setAsAlternate` / `AnalogIn`); host-tested against a fake register block in `test/native_stubs/stm32f4xx.h` |
| `pid/` | PID controller with feedforward, anti-windup, dead-zone boost |
| `power_board/` | UART protocol to rosbot_xl power board MCU (battery state) |
| `range/` | `RangeInterface` + `RangeVl53l0x` + `RangeArray` (rosbot only) |
| `imu_calibration/` | Runtime calibration: session flag (LED), save request serviced from `imuTask` — see "IMU calibration" |
| `time_base/` | `time_boot_us()` / `time_boot_ms()` — the one monotonic clock behind every MAVLink timestamp, TIMESYNC, publisher period and watchdog; safe from any task/ISR and before the scheduler. Extends the 32-bit TIM5 counter (`platform::clock::counterUs()`) to 64 bits; the counter is read before the critical section so a preempting caller is detected by the signed delta |
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
| MotorControl | CONTROL | 200 | Encoder + PID + PWM + current/back-EMF estimation |
| Range | SENSORS | 10 | Read 4× VL53L0X |
| Link | COMMUNICATION | 1000 | `g_mavlink_node.loop()` — MAVLink RX/TX, heartbeat, publishers |

### rosbot_xl tasks

| name | priority | freq [Hz] | role |
|---|---|---|---|
| HwMonitor | OBSERVING | 10 | Battery (via PowerBoard UART) + fan + LED status, rate-limited internally |
| Imu | SENSORS | 100 | same as rosbot |
| LedStrip | OBSERVING | 30 | Drain `led_strip_queue`, render via SPI; idle animation blocks for seconds |
| MotorControl | CONTROL | 200 | same as rosbot |
| Shutdown | OBSERVING | 3 | Detect graceful shutdown signal from power board, stop scheduler |
| Link | COMMUNICATION | 1000 | same as rosbot |

`Link` runs at 1000 Hz on rosbot_xl, 200 Hz on rosbot (the serial
transport is event-driven — see "Patterns: serial transport").

Queue depths are 1 (`xQueueOverwrite`) for telemetry — newest sample
wins, no buffering. Watchdog on `MotorArray` stops motors after 500 ms
without `setVelocities()` (in `feedWatchdog()`).

`MotorArray::update()` also latches driver nFAULT (2 consecutive cycles
low → brake + `resetController()` every cycle, cleared 500 ms after the pin
releases). Only ROSbot 3 has driver groups; DRV8870 / MAX22205 on rosbot_xl
have no nSLEEP/nFAULT wired to the MCU. The `Link` task's
`CriticalStateReporter` turns a latched fault or a critical battery into
`MAV_STATE_CRITICAL` in HEARTBEAT plus one STATUSTEXT per transition (the
bridge does not read `system_status` yet, so on the ROS side only the
STATUSTEXT log shows it); the
status LED shows SOS for a driver fault. A critical battery only signals —
the percentage is a linear map of one voltage sample, so it does not stop
the motors.

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
   first thing after `boardPeripheralsInit()`, before anything else —
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
   callback, which returns the precomputed bool (no GPIO polling there;
   BootOption has already lit the LED).
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

The transports in `lib/mavlink/transport/` never busy-poll, so the link
task does not burn CPU while waiting:

- **`mavlink_udp_transport`** (rosbot_xl) — uses the lwIP raw API. The
  `udp_recv()` callback (`ethernet_scheduler` task)
  pushes the payload into a FreeRTOS stream buffer; `read()` blocks on
  `xStreamBufferReceive(timeout)`; `write()` calls `udp_sendto()` (LwIP TX
  is already DMA-driven).
- **`mavlink_serial_transport`** (rosbot) — a thin wrapper over
  `platform::Uart` (see "UART driver and DMA stream map" below):
  - **RX**: bytes land in the UART's circular DMA buffer; `read()` drains
    it and, when empty, waits with `vTaskDelay(1)` (tick-wrap-safe via
    `vTaskSetTimeOutState` / `xTaskCheckForTimeOut`). `MavlinkNode` only
    reads with a zero timeout from the periodic link task, so no USART
    interrupt is needed.
  - **TX**: `Uart::writeFrame()` queues the whole frame in the UART's stream
    buffer or drops it (never split, never blocks); DMA drains the queue in
    256 B chunks.

### Fatal errors

Every unrecoverable error ends in `fatalError()` (`lib/fault/`):
`configASSERT`, the FreeRTOS stack-overflow hook (`configCHECK_FOR_STACK_OVERFLOW
2`) and malloc-failed hook, and HardFault/MemManage/BusFault/UsageFault. It
resets the MCU — every pin goes Hi-Z, so the motor drivers stop driving —
after halting on a breakpoint if a debugger is attached. Spinning in place
instead would leave the timers generating the last PWM duty with no
watchdog running.

The fault vectors pass the stacked exception frame to
`faultFromException()` (`boards/stm32f407/system.cpp`). Before resetting,
`fatalErrorWithCode()` leaves a 32-byte marker at the top of CCM RAM
(0x1000FFE0), which the linker script never uses and the startup code never
clears: a `FaultCode` (assert, stack overflow, out of heap, hard / bus /
usage / memory-management fault, NMI, clock, exit), the faulting PC for
exceptions and the task name for a stack overflow. On the next boot
`consumeFaultInfo()` reads it; the MAVLink node reports "MCU reset after a
firmware fault" after the boot banner, and `/diagnostics` shows the cause
(e.g. "hard fault at 0x08012e9e", resolve with `arm-none-eabi-addr2line`).

### Diagnostics (`lib/diagnostics/`, ROSBOT_DIAGNOSTICS)

Once a second the link task sends `ROSBOT_DIAGNOSTICS`; the bridge maps it
to `/diagnostics` (see ROS_API.md for the statuses and thresholds).
`lib/diagnostics` samples the MCU itself (CPU idle from the FreeRTOS
run-time counters between two samples, smallest stack headroom and its
task, heap, reset cause read from `RCC_CSR` at boot and cleared, fault
cause); `MavlinkDiagnosticsPublisher` adds the link counters, and each
board's `mavlink_entities.cpp` fills its hardware part through a function
pointer (IMU, range sensors, I2C bus resets, motor faults, battery, power
board, Ethernet). Modules expose counters only through getters
(`I2c::recoveries()`, `RangeVl53l0x::timeouts()`, `ImuInterface::readErrors()`,
`MotorArray::faultEvents()`, `MavlinkNode::txDropped()/rxErrors()`), so the
library layer does not depend on diagnostics.

### lwIP ownership (rosbot_xl)

lwIP is built with `NO_SYS=1` (`include/lwipopts.h`), so it is not
thread-safe. `ethernet::begin()` (`lib/ethernet/ethernet.cpp`) starts the
`ethernet_scheduler` task (priority 5, every tick) that owns it: RX
(`ethernetif_input`), timers, a LAN9303 link check every 500 ms, and every
raw-API callback run there; no ISR touches lwIP (`ETH_INPUT_USE_IT` is
off). Any other task that calls lwIP must keep `ethernet_scheduler` out —
inside `taskENTER_CRITICAL()` or with the scheduler suspended
(`vTaskSuspendAll()`, which leaves IRQs enabled) — and must not wait for
lwIP to make progress meanwhile: `MavlinkUdpTransport::write()` suspends
the scheduler, `sendShutdownRequest()` in `src/rosbot_xl/rtos.cpp` uses a
critical section. Follow-up work (send, close) belongs in the lwIP
callbacks. Both only work because every lwIP caller runs at a lower
priority than `ethernet_scheduler` (5): a caller at 5 or above could
preempt it inside lwIP, where neither guard helps.

`ethernetif.cpp` (ST, with Husarion's LAN9303 support) is vendored in
`lib/ethernet/`; the rest (static IP with a /24 netmask and `.1` gateway,
one `dhcp_inform`) lives in `ethernet.cpp`. HAL ETH is the Legacy (pre-1.27
API) driver; the LAN9303 PHY register remaps it uses live in
`boards/stm32f407/stm32f4xx_hal_conf.h`.

### FreeRTOS-safe IRQ priorities

`configMAX_SYSCALL_INTERRUPT_PRIORITY = 5` (numerical). Cortex-M
convention: lower number = higher priority. Any IRQ that calls
`*FromISR()` API must run at priority **≥ 5** numerically.

NVIC priorities reset to 0 (the highest), above
`configMAX_SYSCALL_INTERRUPT_PRIORITY`: an IRQ enabled without an explicit
priority would crash the kernel on its first `*FromISR` call. Set the
priority before `NVIC_EnableIRQ`, as `lib/platform/` does.

The I2C, UART TX and IMU DMA IRQs in `lib/platform/` are all set to 5,
from init on, for every transfer.

### I2C transfers: IT mode, never polled under the scheduler

Before the scheduler starts (boot, sensor init) transfers are HAL's polled
calls. After that every transfer is HAL IT (`Mem_Write_IT`,
`Mem_Read_IT`, `Master_Transmit_IT`/`Receive_IT`) or, for the 32-byte IMU
block, a sequential IT register write + DMA read, and the calling task
sleeps on a semaphore. Why:

- The polled calls busy-wait the whole transfer, and the BNO055 stretches
  the clock: `HAL_I2C_Mem_Read_DMA`'s polled address phase alone costs
  230-450 us per IMU read (DWT, ROSbot 3), which put the Imu task at
  3.75 % CPU; in IT mode it is 0.43 %.
- The polled calls have steps that must follow within a byte time (for a
  1-byte read, STOP right after ADDR is cleared; HAL 1.8.1 does not mask
  IRQs there). In IT mode the ISR does them.

Event-driven waking of the link task (producers notifying it after each
`xQueueOverwrite`) was tried and dropped: in 4 of 4 sessions a front
VL53L0X stopped answering (NACK at its address, no bus fault) after
motor tests, never with the fixed-rate link loop (12 clean runs). The
cause is not understood; do not reintroduce it without finding it.

### DMA + FreeRTOS handshake

Standard HAL pattern (`platform::I2c` uses it; `platform::Uart` does the same on registers). Steps:

1. `__HAL_RCC_DMAx_CLK_ENABLE()`.
2. Configure `DMA_HandleTypeDef` (direction, increments, sizes).
3. `HAL_DMA_Init`.
4. `__HAL_LINKDMA(periph_handle, hdmarx/hdmatx, our_hdma)`.
5. `HAL_NVIC_SetPriority(stream_irqn, 5, 0)` and enable.
6. Create a binary semaphore (or use `StreamBuffer`).
7. In task: call `HAL_..._DMA(...)` then
   `xSemaphoreTake(timeout)`.
8. Define `extern "C" void DMAx_StreamY_IRQHandler() {
     HAL_DMA_IRQHandler(&our_hdma); }` (the CMSIS startup file declares
   every IRQ vector weak).
9. Override `HAL_..._CpltCallback` (peripheral-specific) to do
   `xSemaphoreGiveFromISR + portYIELD_FROM_ISR`.

`platform::I2c` follows this pattern and also handles
`HAL_I2C_ErrorCallback` (a NACK wakes the reader at once).

### I2C fault recovery

A transfer that ends in a bus fault (HAL timeout, `HAL_BUSY` from a stuck
BUSY flag or a handle left mid-transfer, BERR, ARLO) returns false and then
`I2c::recover()` runs under the bus lock: NVIC off for the bus IRQs,
`HAL_DMA_Abort`, `HAL_I2C_DeInit`, `clearI2cBus()` (`i2c_bus_clear.hpp`: up
to nine 100 kHz SCL pulses on open-drain GPIO until SDA is released, then a
STOP, UM10204 3.1.16), RCC reset and re-init. A plain NACK (`AF`, e.g. a
probe of an absent VL53L0X) does not trigger it. A timed-out read cannot
just call `HAL_I2C_Master_Abort_IT`: HAL 1.8.1 ignores it for a `Mem_Read`
(`MODE_MEM`) and leaves the handle `BUSY_RX`, so one stuck device would
fail every later transfer on the bus (on XL also the EEPROM). `HAL_I2C_DeInit` also clears the handle lock that HAL's
`IsDeviceReady` leaves taken on its `HAL_TIMEOUT` path.

The bus IRQs are enabled only around IT/DMA transfers, and a transfer gets
at most 200 IRQs: past that the ISR switches them off, fails the transfer
and the task resets the bus. Found on ROSbot 3 with the BNO055 unpowered:
it still answered the boot probe through its I/O pins, the first DMA read
then left an event flag HAL never clears, and the re-entering IRQ starved
every task (link silent, no reset). `ImuBno055` also backs off to one read
a second after 10 failures in a row.

`armTransfer()` clears the RX DMA handle's `XferCpltCallback` before every
transfer. Only a DMA read that completes clears it in HAL; one that timed
out leaves it set, and HAL's start-bit handler then sets `DMAEN` for the
next IT write, whose TXE it never serves while `DMAEN` is set: the IRQ
budget trips on every IMU read and `recover()` does not help, because the
callback lives in the handle, not in the peripheral. Found on ROSbot XL
(2026-11-20, ST-LINK): the IMU never came up after one boot-time timeout.

In the first ~120 ms after the switch to fusion mode the BNO055 stretches
the clock long enough that 46 bytes took more than 8 ms: with an 8 ms read
limit every XL boot failed ~12 reads (TCIF set, the DMA IRQ pending, i.e.
the transfer ended just after the timeout) and reset the bus each time.
Hence `kReadTimeoutMs = 25` (0 failures, HW 2026-11-20).

### Range sensors that lose power

A brown-out (seen on ROSbot 3 with the wheels stalled against a wall) puts
every VL53L0X back at its default address, and a sensor that NACKs would
make the Pololu driver's wait-for-data loop spin on I2C for its whole
500 ms timeout. How the firmware handles it (HW-tested 2026-11-20 by
pulsing every XSHUT low for 50 ms at runtime):

- `RangeVl53l0x::update()` reads the interrupt status once instead of the
  driver's wait. NACK, or no data for `timeout_ms`, counts as a timeout;
  two in a row mark the sensor lost and it is no longer read. Not waiting
  also keeps the Range task cheap when the sensors' measurement phase
  drifts from the task's, as it does after a re-init (the wait cost
  ~25 % CPU).
- `RangeArray::update(now_ms)` re-runs `init()` for the whole array while a
  sensor is lost, 1 s after the loss and then doubling up to 16 s: a
  brown-out puts every sensor back at the default address, so XSHUT order
  has to hand the addresses out again. A sensor absent since boot never
  triggers it. `init()` replaces the driver object, which otherwise keeps
  the address the previous `setAddress()` gave it.
- Result on HW: loss detected in ~200 ms, all four back ~1 s later, CPU idle
  95.3 % throughout. The re-init runs on the Range task's stack, hence
  `Stack::L` (headroom had dropped to 164 B with `Stack::M`).

### I2C bus speed (400 kHz)

`platform::I2c::init()` sets 400 kHz with the 2:1 fast-mode duty cycle:
with PCLK1 = 42 MHz, CCR 35 gives exactly 400 kHz and stays inside the
fast-mode tLOW/tHIGH minimums (the 16/9 duty would round CCR to 5, i.e.
336 kHz). The bus speed matters for the IMU: the 46-byte read takes
1.6-6.9 ms at 400 kHz (BNO055 clock stretching plus task wake-up jitter)
and 5-9 ms at 100 kHz, too long for a 10 ms task period. `update()` reports
whether it read a fresh sample, and `imuTask` publishes only those.

### Task timing

Checked on HW with every task instrumented (DWT cycle counter, wake-to-wake
period, per-task CPU from the FreeRTOS run-time stats): all tasks hit their
configured rate. Rules that keep it that way:

- Queue-fed MAVLink publishers move their rate gate only on a real
  publish; IMU, joint state and ranges use `period_ms = 0`, because the
  producing task already sets the rate. A gate that moved on an early call
  with an empty queue dropped samples (`_imu/data` at 82 Hz instead of 100
  on ROSbot 3).
- Periodic tasks use `vTaskDelayUntil`, so their period does not grow by
  the execution time.
- `PowerBoard::update()` never blocks: it drains what the UART holds and
  keeps a partial frame, and the reply is picked up on a later tick (see
  "Power board" for the request spacing).
- The run-time stats read `time_boot_us() >> 4` (62.5 kHz); TIM5 is the
  1 MHz system clock.

Checked on HW: motor PWM 20 kHz on all four timers, XL fan PWM 25 kHz,
encoder timers in encoder mode, USART1 923 kbaud for a 921.6 kbaud link
(+0.16%). The rear-panel USART3 on ROSbot 3 runs +1.27% fast — the best
BRR gets at 42 MHz, still inside UART tolerance. The XL LED-strip SPI
asks for 4 MHz and gets 2.625 MHz (the next power-of-two divider).

### Measured on HW against jazzy 2.4.0 (2026-11)

ROSbot 3 (CORE2) and ROSbot XL (Digital Board v1.2, power board v1.3 with
firmware v1.0.4), the same board flashed alternately with jazzy 2.4.0 and
this firmware, wheels in the air unless noted. Identical instrumentation
in both copies; medians of 3–10 runs. CPU idle was cross-checked by
sampling the PC over SWD at steady state: it agrees with the FreeRTOS run
time stats within ~1 point (interrupt time). Full data:
`~/Husarion/reports/rosbot/cube-fw-metrics-2026-11-19.html` (not in the repo).

| Metric | ROSbot 3 jazzy → new | XL jazzy → new |
|---|---|---|
| CPU idle, idle (SWD PC sampling) | 92.5 → 94.4–94.7 % | 90.7 → 94.0 % |
| CPU idle, driving (run-time stats) | 92.6 → 95.9 % | 88.5 → 94.7 % |
| Imu task CPU | 3.75 → 0.43 % (no busy-wait on I2C) | 3.72 → 0.45 % |
| IMU sample → TX queue, mean / max | 5.95 / 10.05 → 3.53 / 6.98 ms | 3.67 / 7.94 → 3.65 / 7.96 ms |
| Wheel feedback → TX queue, mean | 88 → 65 µs | 95 → 78 µs (driving 265 → 96 µs) |
| IMU / feedback rate | 99.8 / 199.6 → 100.0 / 200.0 Hz (crystal) | 99.7 / 199.4 → 100.0 / 200.0 Hz |
| `/battery` | unchanged | 0.9 → 10 Hz (PB firmware >= v1.0.3) |
| Free heap | 89.4 → 102.5 KB | 44.1 → 55.3 KB |
| Static RAM / image | 11.4 → 9.3 KB / 88 → 100 KB | 56.0 → 52.6 KB / 123 → 152 KB |
| TIMESYNC re-seeds in 10 min | ~17 → 0 | ~17 → 0 (clock −9 ppm) |
| Boot to first MAVLink | 5.6 → 5.3 s | 11.3 s both |
| Wheel tracking, watchdog stop, gyro noise | unchanged | unchanged |

- The largest gap between samples on the host is longer (ROSbot 3: IMU
  17.5 → 29.2 ms): the new firmware sends a sample as soon as it is read,
  so the BNO055's variable read time (clock stretching) shows in delivery,
  while jazzy published on a fixed 5 ms grid at the cost of latency.
  Sample timestamps are regular in both.
- No leak: 30 min of wheel commands, LED strip, LEDs, services, UDP
  garbage and a bridge restart every 5 min. ROSbot 3 free heap constant
  at 102 504 B; on XL (over SWD) newlib heap use, the `sbrk` top and the
  lwIP pools did not move. Every allocation happens at startup.
- Direction reversal is the same in both firmware. A step +2 → −2 rad/s
  sent straight to `/_motors/cmd` overshoots to ~−3 rad/s and settles in
  ~110 ms; through the controller (with its acceleration limits) it is
  smooth. On the floor ROSbot 3 pauses 110–250 ms at zero when reversing
  rotation (skid-steer friction, the PID starts from zero): see "Open work".
- Checked by hand on both robots: LEDs, LED strip, buttons, IMU axes
  (REP-103), boot gestures, IMU calibration window, range sensors, Ethernet
  replug, fan against the NTC, power board shutdown request, flashing with
  `rosbot.flash` from the SBC (snap `rosbot` 1.3.1, `jazzy/edge`; 1.3.0
  accepts only firmware v2.2.1), 10 min with the full stack without a
  disconnect, LAN9303 rear port ↔ SBC ~90 Mbit/s.
- Crystal fallback to the RC checked on XL (HSE bypass with no clock); the
  NMI path checked on ROSbot 3. The clock security system trip itself was
  not tested: it needs the crystal stopped in hardware.

### UART driver and DMA stream map

`lib/platform/uart.{hpp,cpp}` (`platform::Uart`) drives USART1/2/3 on
registers: 8N1, BRR computed exactly like HAL's `UART_BRR_SAMPLING16`
(`uart_math.hpp`, host-tested), TX through a FreeRTOS stream buffer drained
by DMA, RX by circular DMA into a 512 B buffer read by polling its counter.
No USART interrupt is enabled: every reader is a periodic task. Text writes (`print`/`printf`, 256 B per call)
block until queued; before the scheduler runs (and while `createTasks()`
has interrupts masked) every write is polled byte by byte.

| UART | Use | TX DMA | RX DMA |
|---|---|---|---|
| USART1 | rosbot SBC link / rosbot_xl debug | DMA2 S7 ch4 | DMA2 S2 ch4 |
| USART2 | rosbot_xl power board | DMA1 S6 ch4 | DMA1 S5 ch4 |
| USART3 | rosbot FTDI debug / alt link | DMA1 S4 ch7 | DMA1 S1 ch4 |

Request mapping cross-checked against ChibiOS's STM32F40x DMA registry
(RM0090 tables 42/43). USART3 TX uses the Stream 4 alternative because
Stream 3 is the BNO055 I2C2 RX stream.

`DMAx_StreamY_IRQHandler` symbols defined in `lib/` (all others stay weak):

| Symbol | Defined by | Reason |
|---|---|---|
| `DMA1_Stream0_IRQHandler` | `platform/i2c.cpp` | I2C1_RX (rosbot range bus, no DMA reads today) |
| `DMA1_Stream2_IRQHandler` | `platform/i2c.cpp` | I2C3_RX — rosbot IMU |
| `DMA1_Stream3_IRQHandler` | `platform/i2c.cpp` | I2C2_RX — rosbot_xl IMU |
| `DMA1_Stream4_IRQHandler` | `platform/uart.cpp` | USART3_TX |
| `DMA1_Stream6_IRQHandler` | `platform/uart.cpp` | USART2_TX |
| `DMA2_Stream7_IRQHandler` | `platform/uart.cpp` | USART1_TX |

The TX stream interrupts on transfer complete and on transfer error (TEIE):
on an error the hardware disables the stream, the handler drops that chunk
and starts the next one, so a bus fault costs one bad frame instead of
stalling TX for good. DMEIE stays off: a direct-mode error does not stop
the stream, and re-arming it then would corrupt the transfer.

RX streams run without an IRQ. Adding a UART: pick streams that do not
collide with this table, add a `kStreams` entry and its TX IRQ handler in
`uart.cpp`, and update both tables here.

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

CMake + Ninja + `arm-none-eabi-gcc` (Ubuntu: `apt install cmake
ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi
libstdc++-arm-none-eabi-newlib`, the last two are only Recommends of the
compiler; CI uses these packages, GCC 13.2). The
project is `cmake/CMakeLists.txt`, not a root `CMakeLists.txt`: colcon
would take the whole repo for one package and stop before
`bridge/rosbot_mavlink_bridge`. `cmake/COLCON_IGNORE` and a
`COLCON_IGNORE` written into every build dir keep colcon out.

```bash
just build rosbot_xl          # or: scripts/build.sh rosbot_xl
cmake -S cmake --preset release && cmake --build build/release
just test                     # host unit tests (CTest)
```

- One preset (`cmake/CMakePresets.json`): `release` → `build/release/`,
  which builds `rosbot.{elf,bin,map}` and `rosbot_xl.{elf,bin,map}` with
  `-O2 -funroll-loops -g`. A separate debug build was dropped (2026-11-20)
  once the debug-only Monitor task was replaced by `/diagnostics`: it
  differed only by `-funroll-loops`, and gdb works on this one.
- `scripts/build.sh <rosbot|rosbot_xl>` builds one board for `just`,
  `flash.sh` and the VS Code tasks, and prints the `.bin` path.
- Third-party sources are pinned in `cmake/dependencies.cmake` and fetched
  at configure time (network needed once per build dir): STM32CubeF4 HAL
  `v1.8.1`, CMSIS device F4 `v2.6.8`, FreeRTOS-Kernel `V10.3.1`, lwIP
  2.2.1 with ST's port headers (`stm32_mw_lwip` `v2.2.1_20250804`), ETL
  `20.49.0`. The FetchContent projects are only downloaded, never added,
  so their own CMake files do not matter.
- Our code builds with `-Wall -Wextra -Wfatal-errors -Wdouble-promotion
  -Wshadow` and is warning-free; vendor code is compiled with `-w`.
- `lib/` is compiled once as OBJECT libraries, not archives: an archive
  member is only linked when referenced, and IRQ handlers are referenced
  by nothing but the weak vector table.
- `set(FW_VERSION "vX.Y.Z")` in `cmake/CMakeLists.txt` carries the
  firmware version (until 2.4.0 `vX.Y.Z-jazzy`). The release workflow
  (`.github/workflows/release.yaml`) bumps it before tagging and publishes
  `rosbot-<tag>.bin` / `rosbot_xl-<tag>.bin` from `build/release/`. No
  distro suffix: one binary serves every ROS distro; `rosbot_utils`
  compares the `FW:` banner with its `FIRMWARE_VERSION` verbatim.
- Host unit tests: `test/CMakeLists.txt` (CTest + Unity 2.6.1, host
  compiler; `just test`, CI job `test-native`): PID, `time_base`, encoder
  counter delta, NTC, battery SoC and thresholds, the power board protocol
  (fake `platform::Uart` defined in the test), LED indicator patterns, boot
  gestures, fan control, and `lib/platform` drivers against fake register
  blocks. `test/native_stubs/` stands in for the CMSIS device header, the
  FreeRTOS headers and the HAL TIM calls `lib/fan` makes; `fake_gpio.hpp`
  latches BSRR writes into ODR, and the critical-section hook lets a test
  simulate preemption. A test that needs a driver's `.cpp` includes it and
  defines whatever that file calls (`time_boot_us()`, `clock::delayMs()`,
  `Uart` methods). Still untested: `persistent_config`, `MavlinkNode`,
  the comm manager handshake, the I2C/IMU path.

### Startup and memory

`Reset_Handler` (CMSIS template) copies `.data`, clears `.bss`, calls
`SystemInit` (FPU, `VTOR` = flash) and the static constructors. The first
constructor (`boardInit`, priority 101) sets NVIC priority grouping 4, runs
`HAL_Init()` and the clock tree, so every other constructor already sees
HAL and 168 MHz. `main()` calls `setup()`
(`src/<variant>/main.cpp`), which ends in `vTaskStartScheduler()`.

Clock: crystal (HSE) → PLL (2 MHz VCO input) → SYSCLK 168 MHz, APB1 42 MHz,
APB2 84 MHz. CORE2 v1.3+ has an 8 MHz crystal (Q1, value not printed on the
schematic, measured with TIM11 on HSE/16), the Digital Board v1.1/v1.2 a
24 MHz one (Q1); `add_firmware(... HSE_HZ ...)` passes it as `HSE_VALUE`
to the only sources that read it (`system.cpp`, `system_stm32f4xx.c`,
`stm32f4xx_hal_rcc*.c`, compiled per board); `stm32f4xx_hal_conf.h` has no
default, so a shared source that used it would not compile. If the crystal
does not start within `HSE_STARTUP_TIMEOUT` (100 ms) the clock falls back
to the internal 16 MHz RC with the same output frequencies, and
`ROSBOT_DIAGNOSTICS.clock_source` reports it (MCU WARN). The clock security
system is on while the crystal runs: if it stops, the NMI resets with
`kFaultClock`. The RC alone drifts ~2000 ppm (see "Time
synchronization").
`SysTick_Handler` increments `HAL_GetTick()` and, once the scheduler runs,
calls the FreeRTOS tick.

Flash: the image may use sectors 0–10 (896 KB); sector 11 belongs to
`lib/persistent_config`, and the linker script fails the link if the image
grows into it. RAM: one heap for libc and FreeRTOS (`pvPortMalloc()` is
newlib `malloc()`), from the end of `.bss` up to a 1 KB main stack at the top
of RAM (and, until the scheduler starts, never past the current SP); the
main stack serves `setup()` and, after the scheduler starts, every ISR. An
allocation from an ISR asserts. Nothing is placed in CCM RAM, which is what lets `lib/fault` keep
its reset marker there; DMA buffers are declared with `alignas(4)` at file
scope and land in regular SRAM.

### Optimisation level

`-O2` (`FW_OPT_LEVEL` cache variable, `-DFW_OPT_LEVEL=s`
for size). Measured on ROSbot 3, link task at 1 kHz (2026-11-19):

| option | image | idle | Link task CPU |
|---|---:|---:|---:|
| `-Os` | 72 KB | 94.9 % | 3.3 % |
| `-O2` | 100 KB | 96.1 % | 2.4 % |

Stack high-water marks shrink a little at `-O2` (Imu 524 → 468 B free,
Link 2744 → 2584 B free), still far from the limit. LTO was not tried.

Build output sizes (GCC 13.2, `-O2`, 2026-11):
- rosbot ~111 KB Flash (12 %), ~9 KB static RAM (7 %)
- rosbot_xl ~152 KB Flash (17 %), ~52 KB static RAM (40 %)

---

## Power board (ROSbot XL)

What is known about the power board (PB) and how this firmware talks to it.
Facts marked *unverified* have not been checked on hardware or in a datasheet.

- **MCU and code.** STM32L051C8T6 (per the PB schematic). Firmware is
  `husarion/rosbot_xl_pb_firmware` (private, C on STM32Cube LL, built with
  PlatformIO there; its board `rhf76_052` is an L052 definition).
- **Link to the digital board (DB).** UART 38400 baud, DB PD5/PD6 to PB USART1
  (PB6/PB7). Frames are ASCII hex with an XOR checksum: `<CC SS args CC>`. The DB
  uses two commands (full protocol: `docs/UART_PROTOCOL.md` in the PB repo): 1 = board info (firmware version `args[1..11]`, hardware revision
  `args[13..24]`), 2 = battery state. The PB never sends on its own. The DB polls
  at 1 Hz, or at the HwMonitor task rate (10 Hz) when the PB reports firmware
  >= v1.0.3 (`PowerBoard::supportsFastPolling()`).
- **Why the old PB is polled slowly.** PB firmware <= v1.0.2 runs everything in one
  loop with a blocking 750 ms delay: a request is parsed at the end of a pass and
  answered in the next one (0.75-1.5 s), only one frame is handled per pass, and
  the frame parser kept its state in a local variable, so a frame that arrived in
  pieces was dropped. This probably explains the "two requests at once lost
  replies" note in `hwMonitorTask` (the 500 ms spacing is kept for old PBs).
  v1.0.3 services UART every millisecond (measured over SWD: reply in 2-4 ms) and
  refreshes voltage and currents every 100 ms; its charger logic still runs every
  750 ms. Up to v1.0.4 the PB leaves the percentage byte (`args[13]`) and the
  capacity (`args[9..10]`) at 0, which is why this DB computes the percentage
  itself (`SocEstimator`). From v1.0.5 (released as v1.1.0) the PB counts SoC and
  sends both (`args[13]` = 255 while unknown); this branch still ignores them.
- **Versions in the field.** A PB v1.3 reported firmware `v1.0.0` (2026-11-09);
  the PB repository says `v1.0.1`. Version strings are copied from fixed buffers
  on the PB, so a short string is followed by stale bytes; the DB keeps only the
  leading printable run (`lib/power_board/version_text.hpp`). Exposed as the
  `_power_board_info` service (`MAV_CMD_USER_3` -> `ROSBOT_POWER_BOARD`).
- **Hardware revisions.** v1.3 and v1.4 have an FT230X on the rear USB-C port,
  wired to PB USART2 (PA2/PA3), with CBUS0 -> BOOT0 and CBUS1 -> NRST (through a
  MOSFET). v1.2 has no FTDI, only an SWD header (CON12), so users cannot update
  it; keep the DB compatible with v1.2 firmware indefinitely.
- **Updating the PB.** The PB powers the SBC and the DB, so resetting its MCU
  drops them; an update has to run from a separate computer plugged into the rear
  USB-C. **The PB's FT230X ships with the FTDI default EEPROM** (measured on a
  v1.3 unit, serial `DK0C4RU8`, 2026-11-09): `cbus_func_0 = TXDEN`,
  `cbus_func_1 = TXLED`. CBUS bit-bang only drives pins configured as GPIO, so
  BOOT0/NRST are not controllable until those two are rewritten to `GPIO` (the
  DB's FT230X already has `GPIO`; `rosbot-snap` `ftdi_eeprom_conf.py` does it).
  Both devices carry the same product string, `FT230X Basic UART`, so select by
  serial, never by `ftdi://ftdi:ft-x:/1`. **A PB update must therefore check the
  FT230X EEPROM and, if CBUS0/1 are not `GPIO`, write them first** (same two
  properties as the DB; keep a backup of the old EEPROM). Measured on the same
  unit with CBUS0/1 set to `GPIO`: BOOT0 high + NRST pulse over CBUS enters the
  ROM bootloader on USART2 at 57600 8E1; `stm32flash` reads
  `Device ID 0x0417 (STM32L05xxx/06xxx)`, bootloader `0x31`, 64 KiB flash. That
  reset powers the DB off (its USB device vanished, no ping), and the PB did not
  power it back on after leaving the bootloader; a power-button press is
  *probably* needed. *Unverified:* how the PB behaves when a USB host supplies
  VBUS on the charging port. WebUSB cannot open an FT230X on Windows while the
  FTDI driver is bound to it.
- **Same trick on the DB.** The DB is flashed the same way: `rosbot_utils`
  `McuManagerFTDI` drives CBUS0 = BOOT0 and CBUS1 = RST through pyftdi, then runs
  `stm32flash`. It opens the FT-X by device, not by pyftdi URL, because the URL
  lookup reads every FTDI device on the host and fails on an unreadable one.

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
  - `transport/mavlink_{serial,udp}_transport.{hpp,cpp}` — `platform::Uart`
    (serial) / lwIP raw API (UDP), see "Patterns".
  - `dialect/rosbot.xml` — dialect source of truth. The mavgen C output
    lives inside the bridge package at
    `bridge/rosbot_mavlink_bridge/mavlink_dialect/` (single canonical
    location); the firmware reaches it via the include path in
    `cmake/CMakeLists.txt`.
- `src/<variant>/main.cpp` — variant entry point.
- `src/<variant>/mavlink_entities.cpp` — MAVLink publisher/subscriber
  registration + the `g_mavlink_node` definition.

### Topology

- **rosbot**: SBC ↔ MCU over Serial1 @ 921600.
- **rosbot_xl**: SBC ↔ MCU over UDP. MCU binds **14555**, sends to peer
  at **14550** (mavros default port layout). Bridge does the
  opposite — binds 14550, sends to 14555 on the MCU IP. The MCU drops
  datagrams from any IP other than `SBC_IP` (the LAN9303 bridges the external
  RJ45 jack onto this subnet); this stops stray traffic, not a deliberate
  spoof — MAVLink 2 signing would be the real fix.

### State machine

`MavlinkNode::loop()` cycles WAITING → CONNECTED → DISCONNECTED:

```
WAITING        send HEARTBEAT 1 Hz; on first peer HEARTBEAT → CONNECTED,
                sending the boot banner STATUSTEXT once
CONNECTED      send HEARTBEAT 1 Hz, TIMESYNC right away and then 0.5 Hz,
                telemetry at the rates below; if no peer HEARTBEAT for
                3 s → DISCONNECTED
DISCONNECTED   reset → WAITING (motor watchdog already stopped wheels
                500 ms after the last command)
```

### Time synchronization

The bridge turns `time_boot_us` into ROS time with `wall = mcu + offset`
(`TimesyncEstimator`, `bridge/.../timesync_estimator.hpp`, unit-tested).

- **Two-way (primary).** Once the MCU is alive the bridge sends
  `TIMESYNC(tc1=0, ts1=t1)` every 200 ms until it has 5 accepted samples
  (or has sent 25 requests without them), then at 1 Hz. The MCU answers with `tc1 = its time_boot_us·1000` and `ts1`
  echoed; the bridge stamps the reply `t4` and takes
  `offset = (t1 + t4)/2 − tc1`, so the symmetric part of the link delay
  cancels. A reply that does not echo the last request is ignored; a sample
  whose RTT exceeds 2× the median of the last 8 is dropped (it sat in a
  queue). Accepted samples are averaged with weight
  `max(timesync_alpha, 1/n)` — a plain mean at first, then an EWMA.
- **Re-seeding.** A HEARTBEAT reconnect (typically an MCU reboot, whose
  clock restarts near 0) resets the estimate, and so do 3 consecutive
  samples more than 50 ms off it (MCU reboot inside the 3 s timeout, SBC
  wall-clock step). Without that the EWMA took ~5 min to follow a reboot.
- **One-way (fallback).** The MCU still sends its own `TIMESYNC(tc1=0)` every
  2 s; the bridge echoes it and, until the first two-way sample, uses
  `bridge_now − ts1`, which includes the whole uplink delay. The first
  two-way sample replaces that estimate outright. This keeps any firmware
  and any bridge version interoperable.
- Not modelled: MCU clock drift and the MCU's TX queueing ahead of the
  reply, which the RTT filter only partly absorbs. On the crystal the MCU
  is within ~10 ppm of the host (XL: −9 ppm over 2 min, DWT vs host clock):
  10 min with no re-seed, stamp gaps exactly 10 / 5 ms, 0.2–2.6 ms total
  drift (HW 2026-11-20). On the internal RC (jazzy, or the fallback) it
  drifted ~1500–2000 ppm: the offset passed the 50 ms re-seed threshold
  every 30–40 s and stamps jumped ~57 ms forward each time.

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
| `battery` status/presence, `battery/charger_type` (rosbot_xl) | `ROSBOT_POWER_SUPPLY` (11005) | with each `BATTERY_STATUS` | bridge wall clock |
| `_mcu_id` (service) | `COMMAND_LONG(MAV_CMD_USER_1)` → `ROSBOT_MCU_ID` (11020) | — | — |
| `_power_board_info` (service, rosbot_xl) | `COMMAND_LONG(MAV_CMD_USER_3)` → `ROSBOT_POWER_BOARD` (11021) | — | — |

### Bridge package

[`bridge/rosbot_mavlink_bridge`](./bridge/rosbot_mavlink_bridge) — single
`ament_cmake` package built for both jazzy and humble out of one source
tree. The dialect headers live inside the package at
[`bridge/rosbot_mavlink_bridge/mavlink_dialect/`](./bridge/rosbot_mavlink_bridge/mavlink_dialect/)
— this is the **canonical** mavgen output location, not a mirror. The
firmware build reads from the same directory via its include path
(`cmake/CMakeLists.txt`), so there is no duplication. Keeping the headers
inside the package makes the bridge self-contained for bloom releases
to rosdistro (the source tarball archives only the package subtree).
Launch files take a `namespace` arg and set `--ros-args -r __ns:=<value>`
on the node, so rclcpp prefixes every relative topic / service with the
namespace the host also sends the MCU in the `NS:` handshake line.

---

## Open work / known limitations

These are documented to avoid re-discovery:

- **Link RX on rosbot is polled by a periodic task.** RX runs on circular
  DMA into a 512 B ring and the link task drains it every period; at
  921600 baud a 5 ms period fills ~460 B, so a longer stall in that task
  can overrun the ring. An IDLE-line IRQ waking the link task would remove
  both the poll and the limit, but changes the task's timing: measure on
  HW first.
- **No SBC IP auto-discovery.** `SBC_IP` and `CLIENT_IP` are hardcoded in
  `include/rosbot_xl/config.hpp`. True auto-discovery options were
  considered (DHCP server on MCU, broadcast announcement protocol, mDNS)
  but not implemented yet — see commit / chat history for trade-offs.
- **Heap instead of static allocation.** Tasks and queues come from
  `pvPortMalloc()` (newlib `malloc()`), all at startup.
  `configSUPPORT_DYNAMIC_ALLOCATION 0` with `xTaskCreateStatic` is a
  separate change.
- **No CubeMX `.ioc`.** Every peripheral is set up in `lib/platform/` and the
  clock tree in `boards/stm32f407/system.cpp`; an `.ioc` could be added as
  pinout documentation only.
- **HAL ETH is the Legacy (pre-1.27) driver**, kept from the Arduino core
  fork. Moving to the new F4 ETH API is a separate change.
- **`Pin` sets very-high output speed on every pin.** Lowering it (EMI) needs
  a measurement on HW.
- **ROSbot 3 pauses at zero when reversing rotation** (110–250 ms on the
  floor, jazzy too): the PID clears its integral at a zero setpoint and
  restarts from zero against skid-steer friction. Friction feed-forward or
  keeping the integral through zero would shorten it.
- **Crystal only verified on CORE2 v1.3+ and Digital Board v1.1/v1.2.** An
  older CORE2 without a crystal falls back to the RC; one with a crystal
  of another frequency would run at the wrong clock.

When closing one of these items, remove the bullet here and add a
matching entry to the relevant "Patterns" section.
