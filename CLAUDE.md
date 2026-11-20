# Claude — working notes for this repo

This is the always-loaded brief for me (Claude Code). The full technical
reference lives in [ARCHITECTURE.md](ARCHITECTURE.md).

## What this repo is, in one line

STM32F407 firmware (STM32CubeF4 HAL + FreeRTOS + MAVLink, CMake) supporting
two variants — **rosbot** and **rosbot_xl**. Shared libraries in `lib/`,
board support (startup, linker script, HAL/FreeRTOS config) in
`boards/stm32f407/`, variant-specific entry/RTOS in `src/{rosbot,rosbot_xl}/`,
variant-specific configuration in `include/{rosbot,rosbot_xl}/`. Working
branch: **devel** (commit directly); `jazzy` is the release branch.

## Workflow — spec first, code second

The user has stated explicitly: **for any non-trivial change, discuss the
specification before writing code.** Default loop:

1. **Read the relevant code first.** Don't propose solutions before
   understanding the existing shape.
2. **Propose a specification** — what changes, why, what the trade-offs
   are.
3. **Surface the risky parts** — what could break, what is unknown, where
   you want a decision from the user.
4. **Wait for green light.** The user may rephrase the constraint, add a
   requirement, or reject the approach entirely.
5. **Then implement.** Verify by building both affected variants.

Exceptions where you can skip the spec step: trivial fixes (typos, an
obvious bug with one right answer), read-only investigation, continuing
work the user just asked you to resume.

Do not spawn sub-agents for typical tasks. The user is on a plan with
spawn cost — handle work inline. Reserve sub-agents for explicitly named
specialists (e.g. independent code review).

`bridge/rosbot_mavlink_bridge` stays C++ (Rafał, 2026-11-20): do not ask
again about rewriting it in Rust.

## Build

```bash
just check                    # before every commit: hooks, both boards, host tests
just build rosbot             # build/release/rosbot.bin
just test                     # host unit tests (CTest, test/)
just flash-swd rosbot_xl      # ST-LINK (OpenOCD, 4 MHz); `just flash` = bootloader via rosbot_utils
just debug-server             # OpenOCD gdb server on :3333
```

Needs `cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi
libstdc++-arm-none-eabi-newlib` (the last two are only Recommends of the
compiler package). The CMake project is in
`cmake/` (a root `CMakeLists.txt` would hide `bridge/` from colcon), see
ARCHITECTURE.md "Build". **After any change in `lib/`, `boards/` or
`cmake/` build BOTH boards** (`just build-all`). CI does it, you should too.

## Testing on hardware

Changes to drivers, timing or the clock need a run on both robots, not only
host tests. Flash with `just flash-swd` when an ST-LINK is attached, or from
the robot's SBC with the snap: put the `.bin` in
`/var/snap/rosbot/current/config_dir/rosbot_utils/firmware/` under the
version name the snap expects and run `sudo rosbot.flash`; flash the factory
image back afterwards. Check `diagnostics` (see ROS_API.md) and the topic
rates. **Never command wheel motion while a cable or charger is attached**;
wheels in the air unless the user says the robot is free on the floor.
Flashing or resetting a robot needs the user's OK.

## Pre-commit and commit hygiene

Hooks are listed in `.pre-commit-config.yaml` (`just lint` runs them all).
**`clang-format`** (Google style) often rewrites files.

If `clang-format` modifies files during commit, the commit fails. Re-stage
the modified files and commit again. **Never bypass with `--no-verify`** —
fix the underlying issue (or accept the formatter's choice).

Commit rules:
- **Branch `devel`** is the working branch (direct commits OK). `jazzy`
  and `main` change only through a PR.
- Title under ~60 chars, present tense ("Add X", "Fix Y"). Body only when
  the *why* is not obvious from the diff.
- No co-author trailer or "Generated with" footer in commits or PRs.

## Communication style with the user

- Reply in the language the user writes in (Rafał: Polish).
- **Code and commit messages stay in English.** Comments too — match
  surrounding style. The existing repo is mostly English with rare Polish
  comments; do not introduce more Polish into source.
- The user often makes manual edits between turns. Always re-read a file
  with `Read` before assuming its current shape matches what you last
  wrote.
- The user wants explicit trade-offs, numerical sanity checks, and honest
  acknowledgement of unknowns. They dislike defensive over-engineering,
  speculative abstractions, and unsolicited "improvements" beyond the
  asked task.

## Updating this knowledge base

The user has asked specifically: **when a new feature lands, update
`ARCHITECTURE.md`.** Update on:

- New library in `lib/` → "Library layer" section.
- New RTOS task or changed priorities → "RTOS tasks" section.
- New architectural pattern (transport, DMA driver, discovery, ...) →
  "Patterns" section.
- Build system changes (presets, flags, pinned dependencies) → "Build" section.
- New ROS interface (topic, service, message type) → also update
  [ROS_API.md](ROS_API.md), which is the user-facing contract.

Do not update for routine bug fixes, parameter tweaks, or refactors that
do not change the conceptual shape.

## Useful files

- [ARCHITECTURE.md](ARCHITECTURE.md) — full technical reference.
- [ROS_API.md](ROS_API.md) — user-facing ROS topic / service contract.
- [README.md](README.md) — high-level project intro.
- [CONTRIBUTING.md](CONTRIBUTING.md) — `just` recipes, VS Code tasks, dev mode.
- [justfile](justfile) — `just --list` for the canonical build / flash recipes
  used on the SBC.
- [scripts/flash.sh](scripts/flash.sh) — thin wrapper around
  `ros2 run rosbot_utils flash_firmware` that picks the right model / port for
  a board (`rosbot` or `rosbot_xl`) and flashes the `.bin` that
  [scripts/build.sh](scripts/build.sh) just built.
- [cmake/CMakeLists.txt](cmake/CMakeLists.txt),
  [cmake/dependencies.cmake](cmake/dependencies.cmake) — build configuration
  and pinned third-party sources.
- [.pre-commit-config.yaml](.pre-commit-config.yaml) — hook definitions.
