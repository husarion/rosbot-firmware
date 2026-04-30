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
| `comm_manager/` | `CommunicationManager` — chooses primary vs diagnostic transport at boot based on push button |
| `eeprom/` | I2C EEPROM driver + `BoardRevision` (revision string read on rosbot_xl) |
| `encoder/` | `EncoderInterface` + `HardwareEncoder` (STM32 timer in encoder mode, x4) |
| `fan/` | Fan + NTC thermistor (rosbot_xl only) |
| `imu/` | `ImuInterface` + `ImuBno055` (BNO055, DMA path — see "Patterns") |
| `indicator/` | Status LED state machine |
| `led_strip/` | APA102-style LED strip over SPI (rosbot_xl only) |
| `motor/` | `MotorInterface`, `MotorHiZ`, `MotorArray` (Hi-Z PWM control) |
| `pid/` | PID controller with feedforward, anti-windup, dead-zone boost |
| `power_board/` | UART protocol to rosbot_xl power board MCU (battery state) |
| `range/` | `RangeInterface` + `RangeVl53l0x` + `RangeArray` (rosbot only) |
| `ros/ros/` | micro-ROS node, publishers, subscribers, services, transports |

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

Measured impact: rosbot_xl `uRos` 69 % → 8 %. rosbot `uRos` 32 % → ~10 %
(approximate, pending fresh measurement after DMA TX landed).

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
DMA path lowers the EV/ER + DMA stream IRQ priority to 5 in init —
canonical pattern in `lib/imu/imu_bno055.cpp`. Replicate this if you add
another HAL-callback-driven path on top of a framework-managed
peripheral.

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

Build output sizes (representative, debug):
- rosbot ~188 KB Flash, ~50 KB RAM
- rosbot_xl ~226 KB Flash, ~93 KB RAM

CCM RAM usage is implicit (compiler may place stack/BSS there). DMA
buffers are explicitly declared with `alignas(4)` at file scope to land
in regular SRAM.

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
