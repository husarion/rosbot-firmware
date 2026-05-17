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

# Show available recipes.
default:
    @just --list

# Build one PlatformIO env (default: $PIO_ENV or rosbot).
build ENV=default_env:
    pio run -e {{ENV}}

# Build all four micro-ROS envs (debug + release × rosbot, rosbot_xl).
build-microros:
    pio run -e rosbot -e rosbot_release -e rosbot_xl -e rosbot_xl_release

# Build all four MAVLink envs (only present on jazzy-mavlink, post-Phase 1).
build-mavlink:
    pio run -e rosbot_mavlink -e rosbot_mavlink_release \
            -e rosbot_xl_mavlink -e rosbot_xl_mavlink_release

# Wipe PlatformIO build outputs.
clean:
    pio run --target clean

# Build + flash one env (delegates to scripts/flash.sh, which wraps rosbot_utils).
flash ENV=default_env:
    ./scripts/flash.sh {{ENV}}

# Build + flash both variants (micro-ROS). Useful for release smoke-tests.
flash-all:
    just flash rosbot
    just flash rosbot_xl

# Run all pre-commit hooks against the working tree.
lint:
    pre-commit run --all-files
