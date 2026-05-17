# Claude — working notes for this repo

This is the always-loaded brief for me (Claude Code). The full technical
reference lives in [ARCHITECTURE.md](ARCHITECTURE.md).

## What this repo is, in one line

STM32F407 firmware (Arduino + STM32FreeRTOS + micro-ROS) supporting two
variants — **rosbot** and **rosbot_xl**. Shared libraries in `lib/`,
variant-specific entry/RTOS in `src/{rosbot,rosbot_xl}/`, variant-specific
configuration in `include/{rosbot,rosbot_xl}/`. Working branch: **jazzy**.

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

## Build

```bash
# debug builds (default)
platformio run -e rosbot
platformio run -e rosbot_xl

# release (smaller, debug stripped, -O2)
platformio run -e rosbot_release
platformio run -e rosbot_xl_release
```

**After any change in `lib/`, `include/config.hpp`, or `platformio.ini`
build BOTH variants.** CI does it, you should too. Variant-only changes
(`src/<variant>/`, `include/<variant>/`) only need the matching env.

## Pre-commit and commit hygiene

Pre-commit hooks (`.pre-commit-config.yaml`):
- `trailing-whitespace`, `end-of-file-fixer`, `check-yaml`,
  `check-added-large-files` (≤500 KB)
- **`clang-format`** with Google style — frequently modifies files
- `codespell`, `ament_copyright`

If `clang-format` modifies files during commit, the commit fails. Re-stage
the modified files and commit again. **Never bypass with `--no-verify`** —
fix the underlying issue (or accept the formatter's choice).

Commit rules:
- **Branch `jazzy`** is the working branch. **Do not commit to `main`.**
- **Do not stage `.vscode/launch.json`** — it is local IDE state, not
  repo state.
- Title under ~70 chars, present tense ("Add X", "Fix Y"). Body explains
  *why*, not just *what*.
- Co-author trailer on Claude-authored commits:
  `Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>`

## Communication style with the user

- The user converses in **English**. Reply in English.
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
- Build system changes (new envs, new build flags) → "Build" section.
- New ROS interface (topic, service, message type) → also update
  [ROS_API.md](ROS_API.md), which is the user-facing contract.

Do not update for routine bug fixes, parameter tweaks, or refactors that
do not change the conceptual shape.

## Useful files

- [ARCHITECTURE.md](ARCHITECTURE.md) — full technical reference.
- [ROS_API.md](ROS_API.md) — user-facing ROS topic / service contract.
- [MAVLINK_MIGRATION.md](MAVLINK_MIGRATION.md) — implementation spec for the
  alternative MAVLink stack that ships alongside the micro-ROS path.
  Active feature work happens on branch `jazzy-mavlink`.
- [README.md](README.md) — high-level project intro.
- [CONTRIBUTING.md](CONTRIBUTING.md) — `just` recipes, VS Code tasks, dev mode.
- [justfile](justfile) — `just --list` for the canonical build / flash recipes
  used on the SBC.
- [scripts/flash.sh](scripts/flash.sh) — thin wrapper around
  `ros2 run rosbot_utils flash_firmware` that picks the right model / port for
  a PlatformIO env and points at the freshly-built `.pio/build/<env>/firmware.bin`.
- [platformio.ini](platformio.ini) — build configuration.
- [.pre-commit-config.yaml](.pre-commit-config.yaml) — hook definitions.

Project memory lives at
`~/.claude/projects/-home-husarion-repo-rosbot-firmware/memory/`.
Use it for facts about the user (preferences, role) and project-specific
feedback that does not belong in the repo. Do not duplicate this CLAUDE.md
into memory.
