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
    # PYTHONHASHSEED pins Python's str-hash; pymavlink embeds it as
    # MAVLINK_*_XML_HASH and would otherwise drift per process.
    PYTHONHASHSEED=0 {{venv}}/bin/mavgen.py \
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
