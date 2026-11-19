# Plan: Arduino → STM32Cube HAL + FreeRTOS + ETL

Status: steps 0–7 done (local `devel`, not HW-tested), 2026-11-19.

Decisions (Rafał, 2026-11-19): CMake + CubeMX after step 9; steps 8–9 on
a short-lived branch merged into `devel` after HW test on both robots;
ETL only where it replaces hand-rolled code.

## Goal

Drop `framework = arduino` (stm32duino + Husarion fork, Adafruit/Pololu
libs, STM32Ethernet/LwIP wrappers). Firmware runs on CubeMX-generated
startup + STM32CubeF4 HAL/LL, the FreeRTOS kernel used directly (no
CMSIS-RTOS), ETL for fixed-capacity containers. No variant selection by
preprocessor.

## Verified constraint: HAL2 does not exist for STM32F4

ST's offer table (github.com/STMicroelectronics/STM32Cube_MCU_Overall_Offer,
checked 2026-11-19) lists HAL2 only for STM32C5, STM32G5 and STM32H7P.
STM32F4 is in the HAL1 table and ST staff said there is no plan to port
older series. So the target is **STM32CubeF4 (HAL1) + LL**, with LL in hot
paths (encoder, PWM, UART DMA). Code is written so a later HAL2 port only
touches `lib/platform/`.

## Findings (what to fix, beyond the framework itself)

Already solved by leaving Arduino (they are listed in ARCHITECTURE "Open
work"):

1. Serial RX on rosbot is a `vTaskDelay(1)` poll: `HardwareSerial::_serial`
   is private and `HAL_UART_RxCpltCallback` is a strong symbol in the core.
   Own UART driver: DMA circular RX + IDLE-line IRQ + task notification.
2. `HAL_I2C_ErrorCallback` is strong in the core, so the BNO055 DMA path
   cannot signal errors and relies on a timeout.
3. `Wire` resets the bus to 100 kHz on `begin()` (the "Wire 100 kHz reset"
   gotcha, `lib/i2c_fast_mode`).
4. Husarion fork of the Arduino core must be maintained only for serial
   buffer sizes and the RMII pin map.
5. Release builds are effectively `-Os`: the platform appends its own flag
   after ours. With our own CMake we choose (and measure) the level.
6. USART IRQ handler symbol collisions with the core (ARCHITECTURE
   "USART → DMA mapping").

To fix during the migration:

7. Heap use after boot is avoidable: `new HardwareTimer` in
   `motor_hi_z.cpp`, `xTaskCreate`/`xQueueCreate` with heap_4. Target:
   `configSUPPORT_DYNAMIC_ALLOCATION 0`, static tasks/queues.
8. ~~Task frequency travels as `float → uintptr_t → uint16_t` through the
   task `void*`, so a fractional rate truncates.~~ Fixed in step 1.
9. Globals reached from `lib/` (`g_battery`, `g_imu`, `g_comm_mgr`,
   `g_motors`, `g_indicator`) couple libraries to the app. Inject them.
10. `src/rosbot/rtos.cpp` and `src/rosbot_xl/rtos.cpp` duplicate most
    tasks (~190 differing lines of ~500). Shared tasks move to a common
    file; boards differ only by the task table and board objects.
11. Shared I2C2 (XL: IMU DMA + EEPROM) has no bus mutex; safe only because
    EEPROM is boot-only. The new I2C driver gets a mutex.
12. `#ifndef RELEASE` for the Monitor task: becomes a CMake option /
    build-type constant (`constexpr bool kDebugBuild`).
13. Repo `CLAUDE.md` is stale against current practice: says working
    branch `jazzy`, "user converses in English", Opus 4.7 co-author trailer.
14. **The MCU runs from its internal RC oscillator, not a crystal.** The
    Arduino generic F407ZG variant's `SystemClock_Config` selects
    `RCC_OSCILLATORTYPE_HSI` (±1 % over
    temperature per the datasheet) → PLL 168 MHz. Every baud rate (921600
    SBC link, power board UART) and timer inherits that tolerance. Check
    the schematics for an HSE crystal before writing the CubeMX clock tree
    in step 9; if one is fitted, use it.
15. **BNO055 gyro scale is unverified.** Adafruit_BNO055 sets UNIT_SEL
    to rad/s (bit 1), while `ImuBno055::update()` converts as if the
    register were 1/16 deg/s. If the rad/s scale is 900 LSB per rad/s,
    `/imu` angular velocity reads ~1.8 % low; Bosch also acknowledges a
    firmware scaling bug in this chip. Step 7 keeps both as they were.
    Settle it on HW: spin the robot a known angle and integrate
    `/imu` angular_velocity.z, then fix UNIT_SEL or the factor in its own
    commit.

## Variant model without `#ifdef`

Done in step 0: no `ROSBOT`/`ROSBOT_XL` macro, no dispatcher, no variant
`#ifdef` in `lib/`.

Target: one CMake executable per board (`rosbot`, `rosbot_xl`), each
linking `app_common` + its `boards/<board>/` package (CubeMX code,
`board.cpp` with object construction, `config.hpp` as `constexpr`
structs). Selection is by link, never by macro.

Single binary with runtime detection was considered and rejected: the
boards use different peripherals (Ethernet + LAN9303 vs UART link, ADC
battery vs power board), different clock/pin init from CubeMX, and XL
already uses 41 % static RAM.

## Phases

Each step keeps all four envs building and the native tests green, so
`devel` stays flashable. Steps marked HW need a run on both robots before
the next step lands.

| # | Step | Risk | HW |
|---|------|------|----|
| 0 | Remove variant macros + `config.hpp` dispatcher | none (machine code identical, verified) | no |
| 1 | ETL (pinned) with first users (`etl::span` in `MotorArray`/`RangeArray`), `platform::Pin` + host tests, tasks get their `TaskConfig*` (finding 8). Time moves to step 3, static RTOS helpers to step 9, where their first users are | low | no |
| 2 | GPIO: `pinMode/digitalWrite/digitalRead` → `platform::Pin` (`Pin{Port::E, 2}` in board configs). Done for every pure-GPIO pin; pins that also feed ADC or PWM (battery, NTC, current sense, fan) stay Arduino until steps 4–5 | low | yes |
| 3 | Time: `platform::clock` on TIM5 (32-bit, 1 MHz) feeds `time_boot_us()`; `millis()` → `time_boot_ms()`, `micros()` → `clock::counterUs()`, `delay()` → `clock::delayMs()` (`vTaskDelay` once the scheduler runs, busy-wait before). Arduino libraries (Adafruit, Pololu, Wire) keep the core's own clock until step 7 | med (time_base invariants, TIMESYNC) | yes |
| 4 | `platform/timer`: shared timer clock enable / kernel clock, `PwmOutput` (registers, same PSC/ARR/PWM1/preload as `HardwareTimer`, host-tested) for the motors; encoders and fan keep their HAL timer code but get pins via `Pin::setAsAlternate` | med (motor control) | yes |
| 5 | `platform::AnalogIn`: ADC1 single-shot on registers with the same setup as `analogRead()` (10-bit, 15 cycles, PCLK2/4), conversion under masked IRQs (XL motor and fan tasks share ADC1). DMA left out: nothing needs it at the current rates | low | yes |
| 6 | `platform::Uart`: registers + DMA TX queue + circular DMA RX for the link, debug and power board UARTs; `HardwareSerial` and `-D ENABLE_HWSERIALx` gone. Finding 1 half closed: no per-byte RX IRQ, but the IDLE-line wakeup waits for step 9 (`USARTx_IRQHandler` is strong in the core's `uart.o`, which `Print.cpp` and Adafruit always link) | high (link) | yes |
| 7 | `platform::I2c` (HAL, bus mutex, DMA read, own IRQs/callbacks), own BNO055 driver, vendored Pololu VL53L0X on it, EEPROM; `platform::Spi` for the LED strip; no source includes `Arduino.h` any more. Wire, Adafruit (BNO055/BusIO/Sensor) and Pololu are out of the link. Closes findings 2, 3, 11 | high (IMU) | yes |
| 8 | Ethernet: STM32Ethernet/LwIP Arduino → HAL ETH (new F4 ETH API) + Cube LwIP + LAN9303 init (XL) | high | yes |
| 9 | Switch-over: CMake + CubeMX `.ioc` per board, FreeRTOS kernel from CubeF4 middleware, linker script (sector 11 reserved, CCM fault marker), static allocation; drop `framework = arduino` | high | yes |
| 10 | Tooling: CI, `justfile`, `scripts/flash.sh`, `release.yaml` (keep artifact names `rosbot-<tag>.bin`), host tests → CMake/CTest + Unity; ARCHITECTURE rewrite | med | no |

Steps 2–8 run inside the Arduino build, using HAL that the core already
links. That is the point: each driver is replaced and HW-tested alone,
instead of one big switch where everything changes at once. The cost:
until step 9, our drivers must avoid colliding with core IRQ handlers and
weak callbacks (same trouble as findings 1, 2, 6), so a few drivers may
only become fully interrupt-driven at step 9.

## Notes for later steps

- `Pin` keeps Arduino's very-high output speed for parity; lowering it
  (EMI) is a separate, HW-measured change.
- `MotorArray` now clamps to `MAX_NUM_MOTORS` (it used to overflow
  `MotorsData` with more than 4 motors; both boards have exactly 4).
- `pin.hpp` picks `stm32_def.h` via `__has_include` while the Arduino core
  is linked (the core's headers break if CMSIS comes first). Drop at step 9.
- Board configs use `using platform::Pin/Port` at global scope; step 9
  moves them into `boards/<board>/` under a namespace.
- Step 2 behaviour differences to watch on HW: outputs are set to their
  level before the pin becomes an output (Arduino switched mode first, so
  e.g. Brake briefly drove the old level); `MotorHiZ::setMode` is much
  cheaper than `pinMode()` (no pin-map search), so the control task gets
  shorter.
- Step 3 behaviour differences: `delay()` in the core is a pure busy-wait
  (STM32FreeRTOS does not override `yield()`), so `delayMs()` inside a task
  now yields the CPU (`ImuBno055::captureCalibrationOffsets` from `imuTask`
  is the only runtime caller). `time_boot_us()` counts from
  `platform::clock::init()` (first line of `setup()`), a few ms later than
  before; MAVLink TIMESYNC absorbs the offset. TIM5 freezes on a debugger
  halt.
- Step 3 fix (folded into its commit before any push): FreeRTOS run-time
  stats used to program TIM5 to 50 kHz in `vConfigureTimerForRunTimeStats`,
  which would have slowed `time_boot_us()` 50x once the scheduler started.
  The hooks now live in `time_base.cpp` and read `time_boot_us() >> 4`
  (16 us per count, wrap after ~19 h instead of ~23.8 h).
- Timer map (both boards): TIM5 clock; TIM10/11/13/14 motor PWM; rosbot
  encoders TIM2/3/4/8; XL encoders TIM1/2/3/4, fan TIM8. STM32Ethernet's
  default TIM14 scheduler is compiled out on XL (`ETHERNET_USE_FREERTOS`).
- After step 5 our own code calls no Arduino GPIO/time/ADC/timer API. Still
  Arduino: `HardwareSerial` (link, debug, power board), `Wire`/`SPI`,
  Adafruit BNO055, Pololu VL53L0X, STM32Ethernet/LwIP, `setup()`/`loop()`
  entry, and the core's clock/HAL init.
- Step 7 behaviour differences: BNO055 init writes the axis remap and clock
  source in the one CONFIG window instead of three more mode switches (same
  final registers, shorter boot); `enableDmaReads()` is gone (DMA works
  right after `init()`); an I2C NACK now ends a DMA read at once instead of
  after the 8 ms timeout; VL53L0X and EEPROM keep Wire's write-STOP-read
  sequence (`I2c::writeThenRead`). Arduino PI/constrain became `kPi` /
  `std::clamp` (same values; `kPi` keeps RAD_PER_TICK bit-identical).
- Step 6 behaviour differences: RX bytes no longer cost an interrupt each
  (921600 baud was up to ~92 k IRQ/s); debug text writes are queued and
  DMA-driven inside tasks, polled before the scheduler; `printf` truncates
  at 255 chars (the Monitor report goes through `print`). BRR is still
  HAL's (USART3 at +1.27 %); plain rounding would give -0.93 %, a separate,
  HW-checked change if ever wanted. Two old `printf` format mismatches
  (`%d` for `BaseType_t`, `%u` for `unsigned long`) surfaced with the
  `format` attribute and are fixed.
- Step 5 behaviour differences: ADC1 is configured once instead of a full
  HAL init/deinit per `analogRead()` (each read drops from tens of us to
  ~1.2 us); reads are now serialized, where XL's motor and fan tasks could
  previously interleave ADC1 re-inits.

## HW smoke test for steps 1–7

Both robots, same firmware build as the commit:

- Boot: red LED on, then status LED behaviour as before (solid until the
  bridge connects, off when OK).
- Boot gestures: tap → diagnostic transport (green confirm), 3 s hold →
  IMU calibration window (red blink, green solid on success).
- Drive forward/back/turn and stop; brake holds; no twitch on mode changes.
  `/joint_states` 100 Hz-class rate unchanged.
- ROSbot: 4 range sensors publish (XSHUT sequencing), both panel LEDs follow
  `ROSBOT_PANEL_LEDS`, both buttons in `ROSBOT_BUTTONS`.
- ROSbot XL: rev 1.1 / 1.2 current limit (ILIM) path, power-board shutdown
  button → shutdown request + confirm line, USB hub alive, audio amp on.
- PWM / ADC (steps 4–5): all four wheels track speed commands smoothly;
  ROSbot `/battery` voltage matches a multimeter as before; XL rev 1.2
  `_motors/feedback` effort follows load (CSO current sense), XL fan speeds
  up with temperature and NTC temperature is plausible.
- UART (step 6): ROSbot bridge connects over the SBC link and stays up for
  10+ min under full telemetry (no MAVLink CRC errors in the bridge log);
  boot handshake (FW/NS/BACKEND/END) works; rear FTDI shows boot text and
  the Monitor report (debug build); FTDI as link via the boot tap; boot
  IMU calibration text on both FTDI and SBC link. XL: debug text on USART1,
  `/battery` from the power board (USART2), PB version service.
- I2C/SPI (step 7): IMU orientation axes unchanged on both robots
  (`/imu` with the robot level, tilted, rotated); `/imu` at its usual rate
  with no timeouts; boot IMU calibration window and saved offsets applied
  on the next boot; runtime calibration save; ROSbot 4 range sensors with
  correct frame per sensor; XL board revision read from EEPROM (ILIM / fan
  path picked accordingly); XL LED strip animations.
- Timing (step 3): bridge TIMESYNC converges and stays (no clock jumps in
  `/imu` / `/joint_states` stamps over 10+ min); wheel watchdog stops the
  robot ~0.5 s after commands stop; runtime IMU calibration
  (`ROSBOT_IMU_CALIBRATION` command) still saves offsets; Monitor task idle
  % vs the 2026-10 baseline (debug build).

## Test strategy

- Per step: build 4 envs, `pio test -e native`, pre-commit.
- Pure refactors: compare release binaries (or normalized disassembly)
  against the parent commit.
- HW steps: smoke on both robots: boot handshake with the bridge,
  `/joint_states` + `/imu` rates (ARCHITECTURE "Timing measured on HW"),
  wheel command response, battery, LEDs, IMU calibration, transport switch
  gesture; stack/CPU numbers from the Monitor task against the 2026-10
  baseline.
