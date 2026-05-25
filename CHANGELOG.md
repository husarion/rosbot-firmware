# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
for the `X.Y.Z` portion of each tag.

Release tags carry a track suffix (`-jazzy` for the micro-ROS track,
`-jazzy-mavlink` for the MAVLink track, etc.). Versions are independent
per track — `v0.1.0-jazzy-mavlink` and `v1.1.0-jazzy` are not comparable
across the suffix boundary, they are parallel release lineages until
MAVLink merges back into `jazzy`.

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

### Migration / setup notes
- Required repo secrets in `rosbot-firmware`: `HUSARION_BOT_APP_ID`, `HUSARION_BOT_PRIVATE_KEY` (PEM private key). GitHub App must be installed on `husarion/rosbot_ros` with `contents:write` + `pull-requests:write`. App tokens are short-lived (1 h), owner-controlled, no annual rotation.
- `rosbot_ros` needs *Allow auto-merge* ON + branch protection on `jazzy-mavlink-v2` with at least one required status check for the auto-merge path to engage.

## [Unreleased] - runtime comm-backend switch

### Changed (breaking)
- **One binary per platform** — `rosbot[_xl]_mavlink*` envs are gone. The single `rosbot_release` / `rosbot_xl_release` binary now contains both micro-ROS and MAVLink stacks; the host driver picks the upstream-link backend at boot via a new `BACKEND:` line in the pre-comm handshake. Operators flashing this release no longer cycle the firmware when switching transports.
- **Release artefacts** drop from four files to two: `rosbot-${TAG}.bin` and `rosbot_xl-${TAG}.bin`. The `rosbot_mavlink-${VERSION}.bin` and `rosbot_xl_mavlink-${VERSION}.bin` artefacts that previously shipped under suffix-free names are no longer produced. Any external consumer pinning those exact names must switch to the unified names; the `-${distro}` suffix on `TAG_NAME` stays because micro-ROS arduino is still jazzy-pinned.
- `lib/mavlink/publishers/*` classes (`BatteryPublisher`, `ImuPublisher`, `JointStatePublisher`, `RangePublisher`, `ButtonsPublisher`) gained a `Mavlink` prefix to clear an ODR collision with the same-named classes in `lib/ros/ros/publishers/*` once both libs link into one image.

### Changed
- Release flow rewritten around `workflow_dispatch` (Actions UI) with a `bump_type` choice input (major/minor/patch) — no more local `just release` + tag push. New `bump-tag` job finds the latest `v*-jazzy` tag, computes the next version, rewrites `FW_VERSION` in `platformio.ini`, commits, pushes, tags, then hands off to the existing build/release steps. GitHub Release notes switch to `generate_release_notes: true` (auto-built from PR history) with the SHA-256 block appended. CHANGELOG.md stays as a manually-maintained historical record — the workflow no longer reads or writes it.

### Added
- `include/comm_backend.hpp` exposes the `CommBackend { MICRO_ROS, MAVLINK }` enum that `CommunicationManager::getSelectedBackend()` returns after the handshake.
- `CommunicationManager::waitForHostConfig` parses `BACKEND:microros|mavlink`, `NS:<namespace>`, and `END` lines in any order. `END` (or timeout) terminates the handshake; both NS and BACKEND are independently optional. Legacy hosts that only emit `NS:` still work, paying a one-time ~2.5 s timeout instead of a fast `END`-driven exit.
- `lib/persistent_config/` stores the last-successful backend + namespace in STM32 flash sector 11 (emulated EEPROM). On boot the values seed the handshake fallbacks; after the handshake settles the manager writes them back only when they actually changed. An MCU reset without a host present therefore reuses the last-known-good config instead of dropping to compile-time defaults. Fresh flash (sector erased to `0xFF`) reads as `MAVLINK` + empty namespace.
- Release pipeline opens an auto-PR against `husarion/rosbot_ros` (`jazzy-mavlink-v2`) after every dispatched release, bumping the firmware pin in `rosbot_hardware.repos`, swapping bundled `.bin` artefacts (prefix-match, also cleans up legacy `_mavlink-` variants), updating `flash_firmware` bundled paths, and bumping the expected-firmware tag in mavlink launch files. The PR carries the `auto-firmware-bump` label so the companion auto-merge workflow squash-merges it on green CI. New job `bump-rosbot-ros` lives at the bottom of [release.yaml](.github/workflows/release.yaml). `rosbot-snap` is intentionally not touched — it sources firmware via `rosbot_ros`, so the snap follows when rosbot_ros cuts its next release.

### Flash budget

STM32F407ZGT6 (1024 KB Flash, 128 KB SRAM):

| variant | Flash | RAM | head |
|---|---:|---:|---:|
| `rosbot_release` | 200 568 B (19.1 %) | 54 164 B (41.3 %) | ~80 % |
| `rosbot_xl_release` | 237 696 B (22.7 %) | 96 376 B (73.5 %) | ~77 % |

Merge cost vs. the previous micro-ROS-only baseline: ~12 KB Flash, ~2 KB RAM per variant.

### Removed
- `just release` recipe + `.release/apply-release.py` helper. The same job (version bump + commit + tag + push) now runs in CI on dispatch; no Claude/jq/pio dependency on the host.

### Internal
- `src/<variant>/main_mavlink.cpp` deleted; `src/<variant>/ros_mavlink.cpp` renamed to `mavlink_entities.cpp`.
- `src/<variant>/rtos.cpp` uses `RoboticsLink* g_link = nullptr` assigned in `main.cpp::setup()` after the handshake, replacing the `#ifdef USE_MAVLINK` dispatch.
- `lib/ros/ros/transport/serial_transport.cpp` defines weak `mavlink_serial_dma{2_stream7,1_stream4}_isr` hooks; `lib/mavlink/transport/mavlink_serial_transport.cpp` provides the strong overrides. The canonical `DMA{2_Stream7,1_Stream4}_IRQHandler` symbols live in lib/ros now and dispatch to both branches.
- `include/<variant>/config.hpp` no longer pulls `lib/ros/ros/publishers/*` (its `BatteryStamped` et al. would clash with `lib/mavlink/mavlink_types.hpp`'s identically-named structs). Publisher configs moved alongside the publishers in `ros.cpp`.

### Migration / setup notes
- Required repo secrets in `rosbot-firmware`: `HUSARION_BOT_APP_ID`, `HUSARION_BOT_PRIVATE_KEY` (PEM private key). GitHub App must be installed on `husarion/rosbot_ros` with `contents:write` + `pull-requests:write`. App tokens are short-lived (1h), owner-controlled, no annual rotation.
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
