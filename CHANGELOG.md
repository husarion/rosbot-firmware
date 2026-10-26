# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
for the `X.Y.Z` portion of each tag.

Release tags carry a track suffix (`-jazzy` for the main track — micro-ROS
until 2.2.0, MAVLink-only since — and `-jazzy-mavlink` for the former
MAVLink preview track). Versions are independent
per track — `v0.1.0-jazzy-mavlink` and `v1.1.0-jazzy` are not comparable
across the suffix boundary; they were parallel release lineages until
MAVLink merged into `jazzy` in 2.0.0.

## [Unreleased]

Fixes from the 2026-10 static firmware audit. Not yet tested on hardware.

### Fixed
- NaN/Inf wheel setpoints are rejected (watchdog not fed); PID and PWM guard against non-finite values.
- ROSbot XL fan runs at 100 % when the NTC reads open or shorted, instead of switching off.
- One race-free monotonic clock (`lib/time_base/`) behind all MAVLink timestamps and TIMESYNC; the old per-file accumulators could double-count under preemption.
- PowerBoard's occasional ~3.39 V frame is dropped before it reaches the battery data (status LED, critical-battery check, back-EMF model).
- ROSbot XL shutdown request to the SBC goes through lwIP's own task, as valid HTTP; `EthernetClient` called lwIP unprotected.
- Boot banner is sent on every new MAVLink connection, so a restarted bridge sees it.
- PID `dt` from `micros()` (was quantised to 1 ms at a 5 ms period).
- Wheel position is rebuilt from an int64 tick count; float accumulation lost ticks once the position grew.
- Fatal errors (assert, stack overflow, malloc failure, HardFault/BusFault/UsageFault) reset the MCU instead of spinning with the motor PWM still running; the next connection reports "MCU reset after a firmware fault".
- Encoder init enables GPIO clocks for every port and asserts on bad timers in release too; LED strip validates its size before storing it; a serial TX frame that does not fit is dropped whole, never split.
- Release builds compiled parts of the tree as C++14 (`build_unflags` override dropped the base `-std=gnu++14` unflag).
- Debug-serial output from several tasks no longer interleaves (shared mutex; the link task never waits on it).
- Bridge: after an MCU reboot the time offset is re-seeded instead of taking ~5 min to converge.

### Added
- Motor driver nFAULT latch (ROSbot 3): brake and reset the controllers while asserted, SOS on the status LED.
- `MAV_STATE_CRITICAL` in HEARTBEAT and a STATUSTEXT per transition for a driver fault or critical battery (signal only).
- Bridge: two-way TIMESYNC with an RTT outlier filter; MCU-initiated TIMESYNC remains as a fallback.
- Host unit tests (`pio test -e native`, `just test`) for PID, `time_base`, encoder counter delta and NTC conversion, run in CI.

### Changed
- ROSbot XL accepts MAVLink over UDP only from `SBC_IP`.
- ROSbot XL has no motor driver groups: PE1 is the hub reset, PC13 the rev 1.1 fan, PC14/PE0 unconnected.
- ROSbot XL `MotorControl` stack 576 B → 1 KB; `LedStrip` task priority 4 → 2.
- The Arduino core fork is pinned to a commit and its 512 B serial RX ring is asserted at build time.
- Removed the no-op `AWAIT_TIMESYNC` link state and `TODO.md`.
- MAVLink RX is drained in 64-byte blocks; the XL UDP write suspends the scheduler instead of masking IRQs.
- Builds use `-Wdouble-promotion -Wshadow`; shared STM32 settings moved from `[env]` to `[stm32]` in `platformio.ini`.
- Documented that release builds are effectively `-Os` (the platform appends it after `-O2`).

## [2.2.1-jazzy] - 2026-09-25

### Fixed
- IMU calibration status is kept once the chip is fully calibrated.

## [2.2.0-jazzy] - 2026-09-23

### Removed
- micro-ROS support; MAVLink + `rosbot_mavlink_bridge` is the only upstream link.

### Fixed
- Dropped IMU samples on ROSbot 3 (publisher gate moved only on a real publish), I2C at a true 400 kHz, non-blocking PowerBoard reads.

## [2.1.0-jazzy] - 2026-09-23

### Added
- Runtime IMU calibration over MAVLink (`ROSBOT_IMU_CALIBRATION`, `MAV_CMD_USER_2` start/stop/save), offsets persisted in flash.
- ROSbot 3 boot-time IMU calibration log relayed over the SBC link.

## [2.0.4-jazzy] - 2026-09-01

### Changed
- Flash size capped at sector 10 so a reflash cannot corrupt the persistent config in sector 11.
- `SHUTDOWN_WAIT_MS` 5 s → 10 s.
- Bridge lint/CI fixes; LAN9303 register access and VLAN trade-off documented.

## [2.0.3-jazzy] - 2026-08-26

### Added
- Boot-time BNO055 calibration (button hold), with the I2C DMA/IRQ conflict fixed.

## [2.0.2-jazzy] - 2026-06-19

### Fixed
- ROSbot 3 drives PC8 to select BNO055 address 0x29.
- Bridge launch forwards `serial_port` / `serial_baudrate`.

## [2.0.1-jazzy] - 2026-06-01

### Changed
- Bridge promotes the link on the first HEARTBEAT instead of the boot banner.
- Board definition renamed to the repo-local `rosbot_stm32f407`; bridge prepared for a bloom release.

## [2.0.0-jazzy] - 2026-05-25

### Added
- **Runtime micro-ROS / MAVLink switch** — single `rosbot_release` / `rosbot_xl_release` binary now contains both stacks. Host driver picks the upstream-link backend at boot via a new `BACKEND:microros|mavlink` line in the pre-comm handshake; no firmware reflash needed when switching transports. Legacy hosts that only emit `NS:` still work, paying a one-time ~2.5 s timeout.
- **Persistent backend + namespace** in STM32 flash (sector 11, emulated EEPROM). On boot the values seed the handshake fallbacks; after the handshake the manager writes them back only when they actually changed. An MCU reset without a host present therefore reuses the last-known-good config instead of compile-time defaults. Fresh flash reads as `MAVLINK` + empty namespace.
- Release pipeline opens an auto-PR against `husarion/rosbot_ros` (`jazzy-mavlink-v2`) after every dispatched release — bumps the firmware pin in `rosbot_hardware.repos`, swaps bundled `.bin` artefacts, updates `flash_firmware` bundled paths, and bumps the expected-firmware tag in mavlink launch files. Labelled `auto-firmware-bump` for the companion auto-merge workflow.

### Changed (breaking)
- **One binary per platform** — `rosbot[_xl]_mavlink*` PlatformIO envs are gone. Operators flashing this release no longer cycle the firmware when switching transports.
- **Release artefacts drop from four files to two**: `rosbot-${TAG}.bin` and `rosbot_xl-${TAG}.bin`. The `rosbot_mavlink-${VERSION}.bin` / `rosbot_xl_mavlink-${VERSION}.bin` artefacts are no longer produced. External consumers pinning those names must switch to the unified names; the `-${distro}` suffix on `TAG_NAME` stays because micro-ROS arduino is still jazzy-pinned.
- **`lib/mavlink/publishers/*` classes gained a `Mavlink` prefix** (`BatteryPublisher` → `MavlinkBatteryPublisher`, etc.) to clear an ODR collision with same-named classes in `lib/ros/ros/publishers/*` once both libs link into one image.

### Changed
- Release flow rewritten around `workflow_dispatch` (Actions UI) with a `bump_type` choice input (major/minor/patch) — no more local `just release` + tag push. New `bump-tag` job finds the latest `v*-jazzy` tag, computes the next version, rewrites `FW_VERSION` in `platformio.ini`, commits, pushes, tags, then hands off to the existing build/release steps. GitHub Release notes switch to `generate_release_notes: true` (auto-built from PR history) with the SHA-256 block appended. CHANGELOG.md stays a manually-maintained historical record.

### Removed
- `just release` recipe + `.release/apply-release.py` helper — the same job (version bump + commit + tag + push) now runs in CI on dispatch; no Claude/jq/pio dependency on the host.

### Flash budget

STM32F407ZGT6 (1024 KB Flash, 128 KB SRAM):

| variant | Flash | RAM | head |
|---|---:|---:|---:|
| `rosbot_release` | 200 568 B (19.1 %) | 54 164 B (41.3 %) | ~80 % |
| `rosbot_xl_release` | 237 696 B (22.7 %) | 96 376 B (73.5 %) | ~77 % |

Merge cost vs. the previous micro-ROS-only baseline: ~12 KB Flash, ~2 KB RAM per variant.

### Internal
- `src/<variant>/main_mavlink.cpp` deleted; `src/<variant>/ros_mavlink.cpp` renamed to `mavlink_entities.cpp`.
- `src/<variant>/rtos.cpp` uses `RoboticsLink* g_link = nullptr` assigned in `main.cpp::setup()` after the handshake, replacing the `#ifdef USE_MAVLINK` dispatch.
- `lib/ros/ros/transport/serial_transport.cpp` defines weak `mavlink_serial_dma{2_stream7,1_stream4}_isr` hooks; `lib/mavlink/transport/mavlink_serial_transport.cpp` provides the strong overrides. The canonical `DMA{2_Stream7,1_Stream4}_IRQHandler` symbols live in lib/ros now and dispatch to both branches.
- `include/<variant>/config.hpp` no longer pulls `lib/ros/ros/publishers/*` (its `BatteryStamped` et al. would clash with `lib/mavlink/mavlink_types.hpp`'s identically-named structs). Publisher configs moved alongside the publishers in `ros.cpp`.

### Migration / setup notes
- Required repo secrets in `rosbot-firmware`: `HUSARION_BOT_APP_ID`, `HUSARION_BOT_PRIVATE_KEY` (PEM private key). GitHub App must be installed on `husarion/rosbot_ros` with `contents:write` + `pull-requests:write`. App tokens are short-lived (1 h), owner-controlled, no annual rotation.
- `rosbot_ros` needs *Allow auto-merge* ON + branch protection on `jazzy-mavlink-v2` with at least one required status check for the auto-merge path to engage.

## [0.1.1-jazzy-mavlink] - 2026-05-18

### Added
- Release pipeline now publishes `rosbot_mavlink_bridge` container images to GHCR alongside the firmware artifacts.

### Fixed
- Release workflow no longer aborts under `set -u` while assembling the `rosbot_mavlink_bridge` tarball, so the bridge artifact is produced reliably.

## [0.1.0-jazzy-mavlink] - 2026-05-18

### Added
- MAVLink firmware as a second transport alongside the micro-ROS firmware; both rosbot and rosbot_xl can be flashed with either flavour. Pairs with the new `rosbot_mavlink_bridge` on the SBC side.
- `just` recipes plus `scripts/flash.sh` wrapper for the SBC-side build + flash workflow; flash picks the right model and port based on the PlatformIO env and points at the freshly-built `firmware.bin`.
- `just release` recipe with tag-driven release workflow — bumps the version, updates the changelog, commits, tags and pushes; auto-bootstraps the dev venv (PlatformIO included) on a clean host.

### Changed
- `ROS_API.md` rewritten to be transport-neutral, so the same topic/service contract covers both the micro-ROS firmware and the MAVLink firmware + `rosbot_mavlink_bridge` pair.
