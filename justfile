# Husarion ROSbot firmware — run `just --list` to see recipes.
#
# `just` is a project-scoped command runner (https://just.systems). Each
# recipe is a small shell script keyed by a name. Compared to a Makefile,
# recipes can take ordered args, the syntax is bash-friendly, and there is
# no implicit dependency-graph behaviour to surprise anyone.
#
# Common usage:
#   just                          # list recipes
#   just build rosbot             # build one board (rosbot or rosbot_xl)
#   just flash rosbot_xl          # build + flash one target
#   just build-all                # build both boards
#   just test                     # host unit tests (CTest)
#   just check                    # lint + both boards + host tests
#   SERIAL_PORT=/dev/ttyUSB1 just flash rosbot_xl   # override port

# Default target if none is passed on the CLI.
default_env := env_var_or_default("FW_TARGET", "rosbot")

# Path to the Python venv where pymavlink lives. `install-deps` creates it;
# `mavgen` runs from it.
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
#   sudo apt install python3-venv python3-full stm32flash \
#     cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi \
#     libstdc++-arm-none-eabi-newlib
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

# Build one board (default: $FW_TARGET or rosbot) into build/release/.
build ENV=default_env:
    #!/bin/bash
    set -euo pipefail
    ./scripts/build.sh "{{ENV}}"

# Build both boards.
build-all:
    #!/bin/bash
    set -euo pipefail
    [[ -f build/release/CMakeCache.txt ]] || cmake -S cmake --preset release
    cmake --build build/release

# Host unit tests for the hardware-free libraries (test/, CTest + Unity).
test:
    #!/bin/bash
    set -euo pipefail
    [[ -f build/test/CMakeCache.txt ]] || cmake -S test -B build/test -G Ninja
    cmake --build build/test
    ctest --test-dir build/test --output-on-failure

# Wipe build outputs (firmware and host tests).
clean:
    #!/bin/bash
    for dir in build/release build/test; do
      [[ -d $dir ]] && cmake --build "$dir" --target clean
    done
    true

# Build + flash one target (delegates to scripts/flash.sh, which wraps rosbot_utils).
flash ENV=default_env:
    #!/bin/bash
    ./scripts/flash.sh {{ENV}}

# Everything CI checks that runs offline: hooks, both boards, host tests.
check: lint build-all test

# Leaves flash sector 11 (saved settings) alone. OPENOCD overrides the binary.
# Build + flash over an ST-LINK (SWD at 4 MHz, ~6 s).
flash-swd BOARD=default_env:
    #!/bin/bash
    set -euo pipefail
    bin="$(./scripts/build.sh "{{BOARD}}")"
    "${OPENOCD:-openocd}" -f interface/stlink.cfg -f target/stm32f4x.cfg \
      -c "adapter speed 4000" -c "program $bin 0x08000000 verify reset exit"

# GDB server on :3333 over an ST-LINK, for gdb-multiarch or VS Code.
debug-server:
    #!/bin/bash
    "${OPENOCD:-openocd}" -f interface/stlink.cfg -f target/stm32f4x.cfg \
      -c "adapter speed 4000"

# Build + flash both variants. Useful for release smoke-tests.
flash-all:
    #!/bin/bash
    just flash rosbot
    just flash rosbot_xl

# Run all pre-commit hooks against the working tree.
lint:
    #!/bin/bash
    pre-commit run --all-files

# Copy release-built MAVLink firmware binaries into the bridge package's
# `firmware/` directory so the rosbot-snap build can bundle them.
# Re-run after every firmware rebuild. The destination is gitignored.
stage-snap-firmware:
    #!/bin/bash
    set -euo pipefail
    dest=bridge/rosbot_mavlink_bridge/firmware
    mkdir -p "$dest"
    for variant in rosbot rosbot_xl; do
      src="$(./scripts/build.sh "$variant")"
      cp -v "$src" "$dest/${variant}_mavlink.bin"
    done
    echo "Firmware binaries staged under $dest — snapcraft can pick them up."

# Cut a release for the current branch (must be `main`). Drives the full
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
    # Stable releases come from RELEASE_BRANCH in release.yaml.
    # Keep this check in sync. Pre-release runs (the `release-pre` recipe)
    # skip this entirely.
    [ "$branch" = "main" ] \
        || { echo "release: must be on 'main' (you're on '$branch'). For a test build use 'just release-pre {{BUMP}}'." >&2; exit 1; }
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
    echo "=== local gate: building release images ==="
    ./scripts/build.sh rosbot >/dev/null
    ./scripts/build.sh rosbot_xl >/dev/null
    echo "build OK"
    echo

    # ---- 3. Commits since last stable tag ----
    # Same lookup as bump-tag in the workflow: v<X.Y.Z> or the legacy
    # v<X.Y.Z>-jazzy, pre-releases (.pre.N) skipped.
    last_tag=$(git tag --list 'v[0-9]*' --sort=-v:refname | grep -v '\.pre\.' \
        | grep -E '^v[0-9]+\.[0-9]+\.[0-9]+(-jazzy)?$' | head -n1 || true)
    if [ -n "$last_tag" ]; then
        range="${last_tag}..HEAD"
        range_desc="since ${last_tag}"
    else
        range=""
        range_desc="full history (first release)"
    fi
    commits=$(git log ${range:+$range} --no-merges --pretty='%h %s' 2>/dev/null || true)
    [ -n "$commits" ] \
        || { echo "release: no commits ${range_desc} — nothing to release." >&2; exit 1; }
    current_version=$(sed -nE 's/^set\(FW_VERSION "([^"]+)"\)/\1/p' cmake/CMakeLists.txt | head -n1)
    echo "=== ${range_desc} (current FW_VERSION: ${current_version}) ==="
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
                    printf 'for ROSbot 3 and ROSbot XL, MAVLink link to the SBC).\n\n'
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
    base=${last_tag:-v0.0.0}
    base_num=$(echo "$base" | sed -E 's/^v//; s/-.*$//')
    IFS='.' read -r maj min pat <<< "$base_num"
    case "$bump" in
        major) maj=$((maj+1)); min=0; pat=0 ;;
        minor) min=$((min+1)); pat=0 ;;
        patch) pat=$((pat+1)) ;;
    esac
    new_tag="v${maj}.${min}.${pat}"
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
# just main). Tag suffixed with `.pre.<run_number>`, the GitHub Release
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
