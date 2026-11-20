#!/usr/bin/env bash
# Build and flash firmware for a given target.
#
# Delegates the STM32 bootloader handshake + stm32flash invocation to
# `ros2 run rosbot_utils flash_firmware` — the same proven entry point the
# rosbot-snap uses (snap/local/flash_launcher.sh). This wrapper only:
#   - builds the binary with scripts/build.sh (CMake)
#   - picks the right --robot-model and --usb flag from the target name
#   - auto-detects the FTDI tty (VID:PID 0403:6015) when --usb is needed
#
# Usage: scripts/flash.sh <target>
#   e.g. scripts/flash.sh rosbot
#        scripts/flash.sh rosbot_xl
#
# Environment overrides:
#   SERIAL_PORT     — skip auto-detection, use this tty
#   SERIAL_TYPE_USB — set to "True" to force USB flashing on rosbot (matches
#                     the snap config key driver.serial-type-usb)
#
# Requires: what scripts/build.sh needs, ros2 with rosbot_utils on the ROS
# path.

set -euo pipefail

ENV_NAME="${1:-}"
if [[ -z "$ENV_NAME" ]]; then
    cat >&2 <<USAGE
Usage: $0 <target>

Valid targets:
  rosbot
  rosbot_xl
USAGE
    exit 2
fi

# Map target name → rosbot_utils robot-model arg.
case "$ENV_NAME" in
    rosbot_xl*) MODEL="rosbot_xl" ;;
    rosbot*)    MODEL="rosbot" ;;
    *)
        echo "ERROR: cannot infer robot model from target '$ENV_NAME'" >&2
        exit 1
        ;;
esac

# rosbot_xl always uses USB/FTDI; rosbot defaults to UART/GPIO unless the
# operator forces USB via SERIAL_TYPE_USB=True (mirrors the snap key
# driver.serial-type-usb consumed by flash_launcher.sh).
USB_FLAG=""
if [[ "$MODEL" == "rosbot_xl" ]] || [[ "${SERIAL_TYPE_USB:-False}" == "True" ]]; then
    USB_FLAG="--usb"
fi

# When flashing over USB, find the FTDI tty. Same VID:PID the snap looks
# for (FT-X family on Husarion robots). Allow override via SERIAL_PORT.
PORT=""
if [[ -n "$USB_FLAG" ]]; then
    PORT="${SERIAL_PORT:-}"
    if [[ -z "$PORT" ]]; then
        # Mirror rosbot_utils.utils.find_device_port — lookup by udev attrs.
        PORT="$(python3 - <<'PY' 2>/dev/null || true
import pyudev
ctx = pyudev.Context()
for d in ctx.list_devices(subsystem="tty"):
    if d.get("ID_VENDOR_ID") == "0403" and d.get("ID_MODEL_ID") == "6015":
        print(d.device_node)
        break
PY
)"
    fi
    if [[ -z "$PORT" || ! -e "$PORT" ]]; then
        echo "ERROR: no FTDI (0403:6015) tty device found." >&2
        echo "  Plug in the USB cable, set SERIAL_PORT=/dev/ttyUSBx, or" >&2
        echo "  install udev rules: ros2 run rosbot_utils install_udev_rules" >&2
        exit 1
    fi
    echo "Using serial port: $PORT"
fi

echo "Building '$ENV_NAME'..."
BIN_PATH="$("$(dirname "${BASH_SOURCE[0]}")/build.sh" "$ENV_NAME")"

# Delegate flashing to the proven rosbot_utils entry point.
if ! command -v ros2 >/dev/null; then
    echo "ERROR: 'ros2' not on PATH. Source a ROS 2 environment first" >&2
    echo "  (e.g. source /opt/ros/jazzy/setup.bash and your rosbot_ros overlay)." >&2
    exit 1
fi

echo "Flashing $BIN_PATH (model=$MODEL${USB_FLAG:+, usb})..."
if [[ -n "$USB_FLAG" ]]; then
    ros2 run rosbot_utils flash_firmware \
        --robot-model "$MODEL" \
        --port "$PORT" \
        --usb \
        --file "$BIN_PATH"
else
    ros2 run rosbot_utils flash_firmware \
        --robot-model "$MODEL" \
        --file "$BIN_PATH"
fi

echo "Done."
