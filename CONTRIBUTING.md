# Developer info and tools

The software uses RTOS tasks to manage individual board peripherals.

## Command-line workflow with `just`

[`just`](https://just.systems) is a project-scoped command runner. The repo
ships a [`justfile`](justfile) with the most common build / flash recipes.
Run `just` (no args) for a recipe list.

Most-used recipes:

```bash
just build rosbot                # Build one PlatformIO env
just build-microros              # Build all four micro-ROS envs
just build-mavlink               # Build all four MAVLink envs (jazzy-mavlink branch)
just flash rosbot_xl             # Build + flash on the connected robot
just flash rosbot_xl_mavlink     # Same, for the MAVLink build
just clean                       # Wipe PlatformIO build outputs
just lint                        # Run pre-commit hooks against the tree
```

Flashing delegates to `ros2 run rosbot_utils flash_firmware` (the same entry
point used by [rosbot-snap](https://github.com/husarion/rosbot-snap)'s
`flash_launcher.sh`), via the wrapper in [`scripts/flash.sh`](scripts/flash.sh).
The wrapper:

- maps the PlatformIO env name to the `--robot-model` argument
  (`rosbot_xl*` → `rosbot_xl`, otherwise `rosbot`),
- auto-selects USB/FTDI flashing for ROSbot XL (or when `SERIAL_TYPE_USB=True`),
- auto-detects the FTDI tty by USB VID:PID `0403:6015` (override with
  `SERIAL_PORT=/dev/ttyUSBx`),
- points `flash_firmware` at the freshly built `.pio/build/<env>/firmware.bin`
  instead of the binary bundled in `rosbot_utils`.

Prerequisites on the ROSbot SBC:

- `platformio` on `PATH` (`pip3 install -U platformio`).
- A sourced ROS 2 environment with `rosbot_utils` available (i.e. the
  `rosbot_ros` overlay built and sourced, or run from inside the snap).
- For USB flashing: udev rules from `ros2 run rosbot_utils install_udev_rules`
  (snap installs these on `post-refresh`).

## VS Code Tasks

To simplify the development process, we have prepared a set of VS Code tasks that can be used to build and flash the firmware.

To use these tasks, open the Command Palette (Ctrl+Shift+P) and search for "Run Task". You will see a list of available tasks, including:

- **ROSbot: Build firmware**
- **ROSbot (Debug): Build and deploy firmware**
- **ROSbot (Release): Build and deploy firmware**
- **ROSbot XL: Build firmware**
- **ROSbot XL (Debug): Build and deploy firmware**
- **ROSbot XL (Release): Build and deploy firmware**
- **Build Releases**

## Dev mode

To enable the development mode, which **allows you to connect via USB FTDI port**, press the **user button** while powering the MCU. The green LEDs will light up.
