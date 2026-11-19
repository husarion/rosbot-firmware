# Plan: Arduino → STM32Cube HAL + FreeRTOS + ETL

Status: draft, 2026-11-19. Branch: `devel`.

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
8. Task frequency travels as `float → uintptr_t → uint16_t` through the
   task `void*` (`include/rtos.hpp`), so a fractional rate truncates.
   Pass a `const TaskConfig*` instead.
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
| 1 | Add ETL (pinned) and `lib/platform/` skeleton: `Pin`, `time` (DWT/TIM µs clock), `Mutex`/static task helpers on FreeRTOS | low | no |
| 2 | GPIO: `pinMode/digitalWrite/digitalRead` → `platform::Pin` (Arduino pin ints → `{GPIOx, GPIO_PIN_n}` in board configs) | low | yes |
| 3 | Time: `millis/micros/delay` → `platform::time` / `vTaskDelay` | med (time_base invariants, TIMESYNC) | yes |
| 4 | PWM + encoder timers: `HardwareTimer` → HAL/LL TIM (motor_hi_z, fan) | med (motor control) | yes |
| 5 | ADC (battery, current sense, NTC) → HAL ADC + DMA | low | yes |
| 6 | UART: `HardwareSerial` → own DMA driver (link, debug, power board); closes finding 1 | high (link) | yes |
| 7 | I2C/SPI: `Wire`/`SPI` → own drivers with bus mutex; BNO055 driver replaces Adafruit; VL53L0X port onto our I2C; LED strip SPI | high (IMU) | yes |
| 8 | Ethernet: STM32Ethernet/LwIP Arduino → HAL ETH (new F4 ETH API) + Cube LwIP + LAN9303 init (XL) | high | yes |
| 9 | Switch-over: CMake + CubeMX `.ioc` per board, FreeRTOS kernel from CubeF4 middleware, linker script (sector 11 reserved, CCM fault marker), static allocation; drop `framework = arduino` | high | yes |
| 10 | Tooling: CI, `justfile`, `scripts/flash.sh`, `release.yaml` (keep artifact names `rosbot-<tag>.bin`), host tests → CMake/CTest + Unity; ARCHITECTURE rewrite | med | no |

Steps 2–8 run inside the Arduino build, using HAL that the core already
links. That is the point: each driver is replaced and HW-tested alone,
instead of one big switch where everything changes at once. The cost:
until step 9, our drivers must avoid colliding with core IRQ handlers and
weak callbacks (same trouble as findings 1, 2, 6), so a few drivers may
only become fully interrupt-driven at step 9.

## Open decisions (Rafał)

1. Build system after step 9: CMake + CubeMX (recommended; ST's current
   toolchain, VS Code extension, no platform-appended flags) vs PlatformIO
   `framework = stm32cube` (keeps `pio`, but its CubeF4 package lags and
   FreeRTOS/LwIP middleware has to be wired by hand).
2. Where steps 8–9 land: directly on `devel` (allowed) or a short-lived
   branch merged after HW test on both robots (recommended: these two
   steps cannot stay green incrementally).
3. ETL scope: containers/strings/delegates where they replace hand-rolled
   code (fixed arrays in `*Array`, function pointers, ring buffers), not a
   blanket rewrite.

## Test strategy

- Per step: build 4 envs, `pio test -e native`, pre-commit.
- Pure refactors: compare release binaries (or normalized disassembly)
  against the parent commit.
- HW steps: smoke on both robots: boot handshake with the bridge,
  `/joint_states` + `/imu` rates (ARCHITECTURE "Timing measured on HW"),
  wheel command response, battery, LEDs, IMU calibration, transport switch
  gesture; stack/CPU numbers from the Monitor task against the 2026-10
  baseline.
