# Husarion ROSbot firmware — run `just --list` to see recipes.
#
# `just` is a project-scoped command runner (https://just.systems). Each
# recipe is a small shell script keyed by a name. Compared to a Makefile,
# recipes can take ordered args, the syntax is bash-friendly, and there is
# no implicit dependency-graph behaviour to surprise anyone.
#
# Common usage:
#   just                          # list recipes
#   just build rosbot             # build one env
#   just flash rosbot_xl          # build + flash one env
#   just build-microros           # build all 4 micro-ROS envs
#   just build-mavlink            # build all 4 MAVLink envs (jazzy-mavlink branch)
#   SERIAL_PORT=/dev/ttyUSB1 just flash rosbot_xl   # override port

# Default PlatformIO env if none is passed on the CLI.
default_env := env_var_or_default("PIO_ENV", "rosbot")

# Path to the Python venv where pymavlink + platformio live. `install-deps`
# creates it; downstream recipes (mavgen, build, flash) prepend it to PATH.
venv := env_var_or_default("ROSBOT_FW_VENV", "$HOME/.venv-rosbot-fw")

# Show available recipes.
default:
    #!/bin/bash
    @just --list

# One-time dev-environment bootstrap. Creates a Python venv with the build +
# code-generation tooling so a fresh ROSbot SBC (or any developer machine)
# can run `just build` / `just flash` / `just mavgen` without manual setup.
#
# Apt prerequisites (need root on Ubuntu 24.04):
#   sudo apt install python3-venv python3-full stm32flash
# `stm32flash` is bundled with rosbot-snap on production robots; install it
# explicitly here for developer machines.
install-deps:
    #!/bin/bash
    set -euo pipefail
    if [[ ! -d {{venv}} ]]; then
      python3 -m venv {{venv}}
    fi
    {{venv}}/bin/pip install --upgrade pip
    # pymavlink is pinned so locally-generated dialect headers stay
    # byte-identical to what CI's verify-dialect job expects. Bumping the
    # pin should accompany a `just mavgen` + re-commit.
    {{venv}}/bin/pip install \
      platformio \
      'pymavlink==2.4.49' \
      pyudev \
      pyserial
    echo
    echo "Dev tools installed in {{venv}}."
    echo "Add to PATH for an interactive shell:"
    echo "  export PATH={{venv}}/bin:\$PATH"

# Regenerate the C MAVLink dialect headers from rosbot.xml. Re-run any time
# you edit lib/mavlink/dialect/rosbot.xml; commit the generated/ diff
# alongside the XML change.
mavgen:
    #!/bin/bash
    set -euo pipefail
    if [[ ! -x {{venv}}/bin/mavgen.py ]]; then
      echo "ERROR: pymavlink not in {{venv}}. Run: just install-deps" >&2
      exit 1
    fi
    {{venv}}/bin/mavgen.py \
      --lang=C --wire-protocol=2.0 \
      --output=lib/mavlink/dialect/generated \
      lib/mavlink/dialect/rosbot.xml
    echo "Regenerated lib/mavlink/dialect/generated/. Commit the diff."

# Build one PlatformIO env (default: $PIO_ENV or rosbot).
build ENV=default_env:
    #!/bin/bash
    pio run -e {{ENV}}

# Build all four micro-ROS envs (debug + release × rosbot, rosbot_xl).
build-microros:
    #!/bin/bash
    pio run -e rosbot -e rosbot_release -e rosbot_xl -e rosbot_xl_release

# Build all four MAVLink envs (only present on jazzy-mavlink, post-Phase 1).
build-mavlink:
    #!/bin/bash
    pio run -e rosbot_mavlink -e rosbot_mavlink_release \
            -e rosbot_xl_mavlink -e rosbot_xl_mavlink_release

# Wipe PlatformIO build outputs.
clean:
    #!/bin/bash
    pio run --target clean

# Build + flash one env (delegates to scripts/flash.sh, which wraps rosbot_utils).
flash ENV=default_env:
    #!/bin/bash
    ./scripts/flash.sh {{ENV}}

# Build + flash both variants (micro-ROS). Useful for release smoke-tests.
flash-all:
    #!/bin/bash
    just flash rosbot
    just flash rosbot_xl

# Run all pre-commit hooks against the working tree.
lint:
    #!/bin/bash
    pre-commit run --all-files

# Build + start the MAVLink bridge container in the background. The
# container uses host networking + IPC so it joins the same DDS domain as
# the rosbot snap and receives MAVLink UDP frames from the firmware.
bridge-up:
    #!/bin/bash
    docker compose -f bridge/docker-compose.yaml up -d --build

# Tail bridge container logs (Ctrl-C to detach).
bridge-logs:
    #!/bin/bash
    docker compose -f bridge/docker-compose.yaml logs -f

# Stop and remove the bridge container.
bridge-down:
    #!/bin/bash
    docker compose -f bridge/docker-compose.yaml down

# One-shot rebuild of the bridge image (no cache) — use after edits to
# bridge/ or lib/mavlink/dialect/.
bridge-rebuild:
    #!/bin/bash
    docker compose -f bridge/docker-compose.yaml build --no-cache

# Cut a release for the current branch. Drives the full local→published
# flow: sanity gate, build all four release envs, ask Claude for a semver
# bump + Keep-a-Changelog section, show the diff, then on y/N confirm
# commit + tag + push branch + push tag. CI (`.github/workflows/release.yaml`)
# takes over on the tag push and publishes the artefacts, reading the
# release body from the CHANGELOG.md section the recipe just wrote.
#
# The y/N gate is the one chance to back out before anything irreversible
# happens. Saying 'y' commits, tags, and pushes in the same breath — push
# is part of the release, not a separate step the operator has to remember.
#
# Requires: claude CLI, jq, python3 — all already present on the SBC.
release:
    #!/bin/bash
    set -euo pipefail
    # Auto-bootstrap the dev venv if it's missing or incomplete. Makes
    # `just release` self-sufficient on a clean host — operator doesn't
    # need to know about `install-deps` first. Idempotent: skipped when
    # the venv already has pio.
    if [[ ! -x {{venv}}/bin/pio ]]; then
        echo "release: 'pio' not in {{venv}} — bootstrapping dev deps..."
        just install-deps
    fi
    export PATH="{{venv}}/bin:$PATH"

    # ---- 1. sanity ----
    [ -z "$(git status --porcelain)" ] \
        || { echo "release: working tree dirty — commit or stash first" >&2; exit 1; }
    command -v pio >/dev/null \
        || { echo "release: 'pio' still not on PATH after install-deps" >&2; exit 1; }
    branch=$(git branch --show-current)
    [ "$branch" != "main" ] \
        || { echo "release: must not be on 'main' (jazzy / jazzy-* is the working branch)" >&2; exit 1; }
    if git remote get-url origin >/dev/null 2>&1 \
       && git fetch --quiet origin "$branch" 2>/dev/null \
       && git rev-parse --verify "origin/$branch" >/dev/null 2>&1; then
        [ "$(git rev-parse HEAD)" = "$(git rev-parse "origin/$branch")" ] \
            || { echo "release: local '$branch' is not in sync with origin/$branch (push or rebase first)" >&2; exit 1; }
    else
        echo "release: warning — couldn't compare against origin/$branch (skipping sync check)"
    fi
    command -v claude >/dev/null \
        || { echo "release: 'claude' CLI not on PATH (https://docs.claude.com/en/docs/claude-code)" >&2; exit 1; }
    command -v jq >/dev/null \
        || { echo "release: 'jq' not on PATH" >&2; exit 1; }

    # ---- 2. distro + commit range ----
    distro=$(echo "$branch" | awk -F'/' '{print $NF}')
    tag_suffix="-${distro}"
    last_tag=$(git tag --list "v*${tag_suffix}" --sort=-v:refname | head -n1 || true)
    if [ -n "$last_tag" ]; then
        range="${last_tag}..HEAD"
        range_desc="since ${last_tag}"
    else
        last_any=$(git tag --list "v*" --sort=-v:refname | head -n1 || true)
        if [ -n "$last_any" ]; then
            range="${last_any}..HEAD"
            range_desc="since ${last_any} (first release on the ${distro} track)"
        else
            range=""
            range_desc="full history (first release ever)"
        fi
    fi
    commits=$(git log ${range:+$range} --no-merges --pretty='%h %s')
    [ -n "$commits" ] || { echo "release: no commits ${range_desc} — nothing to release." >&2; exit 1; }
    current_version=$(sed -nE 's/.*-D FW_VERSION=\\"([^"]+)\\".*/\1/p' platformio.ini | head -n1)
    [ -n "$current_version" ] || { echo "release: couldn't read FW_VERSION from platformio.ini" >&2; exit 1; }

    echo "=== ${distro} ${range_desc} (current FW_VERSION: ${current_version}) ==="
    printf '%s\n' "$commits" | sed 's/^/  /'
    echo

    # ---- 3. local build gate ----
    echo "=== local gate: building all 4 release envs ==="
    pio run -e rosbot_release -e rosbot_xl_release \
            -e rosbot_mavlink_release -e rosbot_xl_mavlink_release
    echo "gate ok"
    echo

    # ---- 4. headless claude → version + changelog section ----
    echo "=== asking claude for version + changelog section ==="
    prompt=$(mktemp); out=$(mktemp); section=$(mktemp)
    trap 'rm -f "$prompt" "$out" "$section"' EXIT
    {
        printf 'You are preparing a release of rosbot-firmware (STM32F4 firmware\n'
        printf 'for ROSbot 3 and ROSbot XL).\n\n'
        printf 'Current FW_VERSION: %s\n' "$current_version"
        printf 'Tag this release will get: vX.Y.Z%s\n\n' "$tag_suffix"
        printf 'Commits to describe (newest first), %s:\n\n' "$range_desc"
        printf '%s\n\n' "$commits"
        printf 'Use the Read tool on CHANGELOG.md to match the existing tone and\n'
        printf 'section style.\n\n'
        printf 'Format: Keep-a-Changelog. Sections inside the entry are\n'
        printf '### Added / Changed / Fixed / Removed (omit empty groups).\n'
        printf 'Bullets must be concise and user-facing. Audience: a robotics\n'
        printf 'engineer reading the release page to decide whether to flash this\n'
        printf 'version. Group related commits; skip pure repo housekeeping\n'
        printf '(lint, template syncs, CLAUDE.md tweaks) unless substantial.\n'
        printf 'Reference flavours by name: "micro-ROS firmware" / "MAVLink\n'
        printf 'firmware" / "rosbot_mavlink_bridge". Reference variants as\n'
        printf '"rosbot" and "rosbot_xl".\n\n'
        printf 'Decide the next semver bump for X.Y.Z:\n'
        printf '  patch -> bug fixes, internal cleanups, doc-only churn\n'
        printf '  minor -> new user-facing features, backwards-compatible additions\n'
        printf '  major -> breaking changes / removals from the public ROS API,\n'
        printf '           wire protocol, or build interface\n\n'
        printf 'Do NOT include the ## header — the recipe prepends it with today\\047s date.\n\n'
        printf 'Your final message MUST be exactly one JSON object, no prose, no\n'
        printf 'code fence, on a single line:\n\n'
        printf '  {"version":"X.Y.Z","section":"### Added\\\\n- foo\\\\n\\\\n### Fixed\\\\n- bar"}\n\n'
        printf 'Newlines inside the section field JSON-escaped as \\\\n.\n'
    } > "$prompt"

    claude -p "$(cat "$prompt")" --allowed-tools Read --output-format json > "$out"
    raw=$(jq -r '.result // empty' "$out")
    [ -n "$raw" ] || { echo "release: claude returned empty result" >&2; cat "$out" >&2; exit 1; }
    raw=$(printf '%s' "$raw" | sed -e '/^```/d')
    version=$(printf '%s' "$raw" | jq -r '.version')
    section_body=$(printf '%s' "$raw" | jq -r '.section')
    [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] \
        || { echo "release: invalid version '$version' from claude" >&2; printf '%s\n' "$raw" >&2; exit 1; }
    [ -n "$section_body" ] || { echo "release: empty section from claude" >&2; exit 1; }

    new_tag="v${version}${tag_suffix}"
    echo "claude proposes: ${current_version} -> ${new_tag}"
    echo

    # ---- 5. apply locally ----
    printf '%s\n' "$section_body" > "$section"
    python3 .release/apply-release.py "$new_tag" "$section"

    # ---- 6. y/N gate, commit + tag ----
    echo
    echo "=== diff for the release commit ==="
    git --no-pager diff --stat
    echo
    git --no-pager diff CHANGELOG.md platformio.ini
    echo
    read -rp "Commit, tag ${new_tag}, and push to origin? [y/N] " confirm
    if [ "${confirm:-N}" != "y" ] && [ "${confirm:-N}" != "Y" ]; then
        echo "aborted — reverting working-tree edits"
        git restore --worktree CHANGELOG.md platformio.ini
        rm -f CHANGELOG.md.bak platformio.ini.bak
        exit 1
    fi

    rm -f CHANGELOG.md.bak platformio.ini.bak
    git add CHANGELOG.md platformio.ini
    git commit -m "Release ${new_tag}"
    git tag -a "${new_tag}" -m "Release ${new_tag}"
    git push --follow-tags origin "${branch}"
    echo
    echo "=== ${new_tag} released — CI is now building artefacts ==="
    if command -v gh >/dev/null 2>&1; then
        repo_url=$(gh repo view --json url -q .url 2>/dev/null || true)
        [ -n "$repo_url" ] && echo "  ${repo_url}/actions"
        [ -n "$repo_url" ] && echo "  ${repo_url}/releases/tag/${new_tag}"
    fi

# Copy release-built MAVLink firmware binaries into the bridge package's
# `firmware/` directory so the rosbot-snap build can bundle them.
# Re-run after every firmware rebuild. The destination is gitignored.
stage-snap-firmware:
    #!/bin/bash
    set -euo pipefail
    dest=bridge/rosbot_mavlink_bridge/firmware
    mkdir -p "$dest"
    for variant in rosbot rosbot_xl; do
      src=".pio/build/${variant}_mavlink_release/firmware.bin"
      if [[ ! -f "$src" ]]; then
        echo "$src missing — building it now..."
        pio run -e "${variant}_mavlink_release"
      fi
      cp -v "$src" "$dest/${variant}_mavlink.bin"
    done
    echo "Firmware binaries staged under $dest — snapcraft can pick them up."
