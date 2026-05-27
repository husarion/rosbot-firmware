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

# Regenerate dialect headers from rosbot.xml. Output goes inside the
# bridge package so bloom tarballs are self-contained; commit the diff
# alongside the XML change.
mavgen:
    #!/bin/bash
    set -euo pipefail
    if [[ ! -x {{venv}}/bin/mavgen.py ]]; then
      echo "ERROR: pymavlink not in {{venv}}. Run: just install-deps" >&2
      exit 1
    fi
    # PYTHONHASHSEED pins Python's str-hash; pymavlink embeds it as
    # MAVLINK_*_XML_HASH and would otherwise drift per process.
    rm -rf bridge/rosbot_mavlink_bridge/mavlink_dialect
    PYTHONHASHSEED=0 {{venv}}/bin/mavgen.py \
      --lang=C --wire-protocol=2.0 \
      --output=bridge/rosbot_mavlink_bridge/mavlink_dialect \
      lib/mavlink/dialect/rosbot.xml
    echo "Regenerated bridge/rosbot_mavlink_bridge/mavlink_dialect/. Commit the diff."

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

# Cut a release for the current branch (must be `jazzy`). Drives the full
# local→dispatched flow: sanity gates → local build gate → (optional, if
# `claude` + `jq` on PATH) Claude-drafted CHANGELOG section + bump
# proposal → y/N → commit + push CHANGELOG → dispatch CI. The Release
# workflow then bumps FW_VERSION, tags, builds + releases, and opens the
# auto-bump PR to rosbot_ros.
#
# Usage:
#   just release             # Claude proposes bump + drafts CHANGELOG (or asks interactively if no claude)
#   just release patch       # explicit bump, skip Claude (CHANGELOG stays as-is)
#   just release minor
#   just release major
#
# Pre-release builds — from any branch, no rosbot_ros bump, no CHANGELOG
# drafting — go through `just release-pre`.
release BUMP="auto":
    #!/bin/bash
    set -euo pipefail

    # ---- 1. Sanity gates ----
    branch=$(git branch --show-current)
    [ -n "$branch" ] \
        || { echo "release: not on a branch (detached HEAD?)" >&2; exit 1; }
    # Stable releases pinned to the EXPECTED_DISTRO branch in release.yaml.
    # Keep this check in sync. Pre-release runs (the `release-pre` recipe)
    # skip this entirely.
    [ "$branch" = "jazzy" ] \
        || { echo "release: must be on 'jazzy' (you're on '$branch'). For a test build use 'just release-pre {{BUMP}}'." >&2; exit 1; }
    [ -z "$(git status --porcelain)" ] \
        || { echo "release: working tree dirty — commit or stash first" >&2; exit 1; }
    command -v gh >/dev/null \
        || { echo "release: 'gh' CLI not installed (https://cli.github.com/)" >&2; exit 1; }
    gh auth status >/dev/null 2>&1 \
        || { echo "release: run 'gh auth login' first" >&2; exit 1; }
    # CI builds from origin/$branch — local must match remote or the release
    # won't reflect what's reviewable on GitHub.
    git fetch --quiet origin "$branch" 2>/dev/null \
        || { echo "release: couldn't fetch origin/$branch" >&2; exit 1; }
    local_sha=$(git rev-parse HEAD)
    remote_sha=$(git rev-parse "origin/$branch")
    [ "$local_sha" = "$remote_sha" ] \
        || { echo "release: local '$branch' ($local_sha) doesn't match origin ($remote_sha) — push or pull first" >&2; exit 1; }

    # ---- 2. Local build gate ----
    # Catches "this commits builds clean locally" before CI burns time.
    # Auto-bootstrap the dev venv so a fresh host doesn't need install-deps
    # as a separate step. Idempotent.
    if [[ ! -x {{venv}}/bin/pio ]]; then
        echo "release: 'pio' not in {{venv}} — bootstrapping dev deps..."
        just install-deps
    fi
    export PATH="{{venv}}/bin:$PATH"
    echo "=== local gate: building release envs ==="
    pio run -e rosbot_release -e rosbot_xl_release
    echo "build OK"
    echo

    # ---- 3. Distro + commits since last stable tag ----
    distro=jazzy  # keep in sync with EXPECTED_DISTRO in release.yaml
    tag_suffix="-${distro}"
    # Exclude .pre.* tags from the "last stable" lookup (same logic as
    # bump-tag in the workflow) so a prior pre-release doesn't shift the
    # baseline.
    last_tag=$(git tag --list "v*${tag_suffix}" --sort=-v:refname | grep -v '\.pre\.' | head -n1 || true)
    if [ -n "$last_tag" ]; then
        range="${last_tag}..HEAD"
        range_desc="since ${last_tag}"
    else
        range=""
        range_desc="full history (first release on the ${distro} track)"
    fi
    commits=$(git log ${range:+$range} --no-merges --pretty='%h %s' 2>/dev/null || true)
    [ -n "$commits" ] \
        || { echo "release: no commits ${range_desc} — nothing to release." >&2; exit 1; }
    current_version=$(sed -nE 's/.*-D FW_VERSION=\\"([^"]+)\\".*/\1/p' platformio.ini | head -n1)
    echo "=== ${distro} ${range_desc} (current FW_VERSION: ${current_version}) ==="
    printf '%s\n' "$commits" | sed 's/^/  /'
    echo

    # ---- 4. Decide bump type (and optionally draft CHANGELOG section) ----
    bump_arg="{{BUMP}}"
    changelog_section_file=""
    case "$bump_arg" in
        auto)
            if command -v claude >/dev/null 2>&1 && command -v jq >/dev/null 2>&1; then
                echo "=== asking claude to propose bump + draft CHANGELOG section ==="
                prompt=$(mktemp); out=$(mktemp); section_file=$(mktemp)
                trap 'rm -f "$prompt" "$out" "$section_file"' EXIT
                {
                    printf 'You are preparing a release of rosbot-firmware (STM32F4 firmware\n'
                    printf 'for ROSbot 3 and ROSbot XL, runtime micro-ROS / MAVLink switch).\n\n'
                    printf 'Current FW_VERSION: %s\n\n' "$current_version"
                    printf 'Commits to describe (newest first), %s:\n\n%s\n\n' "$range_desc" "$commits"
                    printf 'Use the Read tool on CHANGELOG.md to match the existing tone\n'
                    printf 'and section style.\n\n'
                    printf 'Format: Keep-a-Changelog. Subsections: ### Added / Changed /\n'
                    printf 'Fixed / Removed (omit empty groups). Bullets concise and\n'
                    printf 'user-facing — audience is a robotics engineer deciding whether\n'
                    printf 'to flash this version. Group related commits; skip pure repo\n'
                    printf 'housekeeping (lint, template syncs, CLAUDE.md tweaks) unless\n'
                    printf 'substantial.\n\n'
                    printf 'Decide the next semver bump for X.Y.Z:\n'
                    printf '  patch -> bug fixes, internal cleanups, doc-only churn\n'
                    printf '  minor -> new user-facing features, backwards-compatible additions\n'
                    printf '  major -> breaking changes / removals from the public ROS API,\n'
                    printf '           wire protocol, or build interface\n\n'
                    printf 'Do NOT include the ## header — the recipe prepends it with the\n'
                    printf "computed tag and today's date.\n\n"
                    printf 'Output exactly one JSON object on a single line, no prose, no fence:\n'
                    printf '  {"bump":"patch|minor|major","section":"### Added\\\\n- foo\\\\n\\\\n### Fixed\\\\n- bar"}\n'
                } > "$prompt"
                claude -p "$(cat "$prompt")" --allowed-tools Read --output-format json > "$out"
                raw=$(jq -r '.result // empty' "$out")
                [ -n "$raw" ] \
                    || { echo "release: claude returned empty result" >&2; cat "$out" >&2; exit 1; }
                raw=$(printf '%s' "$raw" | sed -e '/^```/d')
                bump=$(printf '%s' "$raw" | jq -r '.bump')
                section_body=$(printf '%s' "$raw" | jq -r '.section')
                [[ "$bump" =~ ^(patch|minor|major)$ ]] \
                    || { echo "release: invalid bump '$bump' from claude" >&2; printf '%s\n' "$raw" >&2; exit 1; }
                [ -n "$section_body" ] \
                    || { echo "release: empty CHANGELOG section from claude" >&2; exit 1; }
                printf '%s\n' "$section_body" > "$section_file"
                changelog_section_file="$section_file"
                echo
                echo "--- proposed CHANGELOG section ---"
                cat "$section_file"
                echo
            else
                echo "release: 'claude' or 'jq' missing — asking interactively"
                read -rp "Bump type [patch/minor/major]: " bump
                case "$bump" in patch|minor|major) ;; *) echo "invalid"; exit 1 ;; esac
            fi
            ;;
        patch|minor|major)
            bump="$bump_arg"
            ;;
        *)
            echo "release: BUMP must be auto|patch|minor|major (got '$bump_arg')" >&2
            exit 1
            ;;
    esac

    # ---- 5. Compute target tag (mirrors workflow's bump-tag) ----
    base=${last_tag:-v0.0.0-${distro}}
    base_num=$(echo "$base" | sed -E "s/^v//; s/${tag_suffix}\$//")
    IFS='.' read -r maj min pat <<< "$base_num"
    case "$bump" in
        major) maj=$((maj+1)); min=0; pat=0 ;;
        minor) min=$((min+1)); pat=0 ;;
        patch) pat=$((pat+1)) ;;
    esac
    new_tag="v${maj}.${min}.${pat}${tag_suffix}"
    echo "=== will dispatch: ${current_version} -> ${new_tag} (bump=${bump}) ==="

    # ---- 6. Apply CHANGELOG (if drafted) ----
    # Inline Python via -c (a heredoc would hit indent problems inside the
    # just recipe — its end marker can't be at column 0 here).
    if [ -n "$changelog_section_file" ]; then
        python3 -c '
    import datetime, pathlib, re, sys
    tag = sys.argv[1]
    key = tag.lstrip("v")
    section = pathlib.Path(sys.argv[2]).read_text().rstrip()
    today = datetime.date.today().isoformat()
    new = f"## [{key}] - {today}\n\n{section}\n\n"
    p = pathlib.Path("CHANGELOG.md")
    text = p.read_text() if p.exists() else "# Changelog\n\n"
    m = re.search(r"(?m)^## \[", text)
    if m:
        text = text[:m.start()] + new + text[m.start():]
    else:
        text = text.rstrip() + "\n\n" + new
    p.write_text(text)
    ' "$new_tag" "$changelog_section_file"
        echo
        echo "--- CHANGELOG.md diff ---"
        git --no-pager diff CHANGELOG.md
    fi

    # ---- 7. y/N gate ----
    echo
    read -rp "Commit CHANGELOG (if any), push, and dispatch CI for ${new_tag}? [y/N] " confirm
    case "${confirm:-N}" in
        y|Y) ;;
        *)
            if [ -n "$changelog_section_file" ]; then
                echo "aborted — reverting CHANGELOG.md"
                git restore --worktree CHANGELOG.md
            else
                echo "aborted"
            fi
            exit 1
            ;;
    esac

    # ---- 8. Commit CHANGELOG + push + dispatch ----
    if [ -n "$changelog_section_file" ] && ! git diff --quiet CHANGELOG.md; then
        git add CHANGELOG.md
        git commit -m "release: CHANGELOG section for ${new_tag}"
        git push origin "$branch"
    fi
    gh workflow run release.yaml \
        --ref "$branch" \
        -f bump_type="${bump}" \
        -f pre_release=false
    echo
    echo "=== Dispatched ${new_tag} — CI building artefacts ==="
    repo_url=$(gh repo view --json url -q .url 2>/dev/null || true)
    if [ -n "$repo_url" ]; then
        echo "  ${repo_url}/actions/workflows/release.yaml"
        echo "  ${repo_url}/releases  (the new release shows up after the workflow finishes)"
    fi
    echo "  gh run watch    # tail progress interactively"

# Dispatch a pre-release build from the current branch (any branch, not
# just jazzy). Tag suffixed with `.pre.<run_number>`, the GitHub Release
# is flagged as pre-release, the rosbot_ros bump PR is SKIPPED. Use to
# test the release pipeline end-to-end before merging infra changes.
#
# Usage: just release-pre [patch|minor|major]   (default: patch)
release-pre BUMP="patch":
    #!/bin/bash
    set -euo pipefail
    case "{{BUMP}}" in
        patch|minor|major) ;;
        *) echo "release-pre: BUMP must be patch|minor|major (got '{{BUMP}}')" >&2; exit 1 ;;
    esac
    command -v gh >/dev/null \
        || { echo "release-pre: 'gh' CLI not installed" >&2; exit 1; }
    gh auth status >/dev/null 2>&1 \
        || { echo "release-pre: run 'gh auth login' first" >&2; exit 1; }
    branch=$(git branch --show-current)
    [ -n "$branch" ] \
        || { echo "release-pre: not on a branch" >&2; exit 1; }
    if [ -n "$(git status --porcelain)" ]; then
        echo "release-pre: warning — working tree dirty; CI builds from origin/$branch"
    fi
    echo "=== Dispatching pre-release ==="
    echo "  branch:    $branch"
    echo "  bump_type: {{BUMP}}"
    read -rp "Continue? [y/N] " confirm
    case "${confirm:-N}" in y|Y) ;; *) echo "aborted"; exit 1 ;; esac
    gh workflow run release.yaml --ref "$branch" \
        -f bump_type={{BUMP}} -f pre_release=true
    echo
    echo "Dispatched. Watch with: gh run watch"
