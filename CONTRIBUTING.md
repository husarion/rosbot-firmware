# Developer info and tools

The software uses RTOS tasks to manage individual board peripherals.

## Command-line workflow with `just`

[`just`](https://just.systems) is a project-scoped command runner. The repo
ships a [`justfile`](justfile) with the most common build / flash recipes.
Run `just` (no args) for a recipe list.

Most-used recipes:

```bash
just build rosbot                # Build one target: rosbot[_xl][_release]
just build-all                   # Both boards, debug + release
just test                        # Host unit tests (CTest)
just flash rosbot_xl             # Build + flash on the connected robot
just clean                       # Wipe CMake build outputs
just lint                        # Run pre-commit hooks against the tree
```

Flashing delegates to `ros2 run rosbot_utils flash_firmware` (the same entry
point used by [rosbot-snap](https://github.com/husarion/rosbot-snap)'s
`flash_launcher.sh`), via the wrapper in [`scripts/flash.sh`](scripts/flash.sh).
The wrapper:

- maps the target name to the `--robot-model` argument
  (`rosbot_xl*` → `rosbot_xl`, otherwise `rosbot`),
- auto-selects USB/FTDI flashing for ROSbot XL (or when `SERIAL_TYPE_USB=True`),
- auto-detects the FTDI tty by USB VID:PID `0403:6015` (override with
  `SERIAL_PORT=/dev/ttyUSBx`),
- builds with [`scripts/build.sh`](scripts/build.sh) and points
  `flash_firmware` at the fresh `build/<debug|release>/<board>.bin` instead of
  the binary bundled in `rosbot_utils`.

Prerequisites on the ROSbot SBC:

- `cmake`, `ninja-build` and the Arm toolchain with newlib
  (`sudo apt install cmake ninja-build gcc-arm-none-eabi
  libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib`). The first build
  downloads the pinned HAL / FreeRTOS / lwIP / ETL sources, so it needs
  network once.
- A sourced ROS 2 environment with `rosbot_utils` available (i.e. the
  `rosbot_ros` overlay built and sourced, or run from inside the snap).
- For USB flashing: udev rules from `ros2 run rosbot_utils install_udev_rules`
  (snap installs these on `post-refresh`).

## VS Code Tasks

To simplify the development process, we have prepared a set of VS Code tasks that can be used to build and flash the firmware.

To use these tasks, open the Command Palette (Ctrl+Shift+P) and search for "Run Task". You will see a list of available tasks, including:

- **ROSbot: Build firmware**
- **ROSbot (Debug): Build and flash firmware**
- **ROSbot (Release): Build and flash firmware**
- **ROSbot XL: Build firmware**
- **ROSbot XL (Debug): Build and flash firmware**
- **ROSbot XL (Release): Build and flash firmware**
- **Build all (debug + release)**
- **Host unit tests**

Each runs the matching `just` recipe; flashing goes through the bootloader
like `just flash`, not ST-Link.

## Dev mode

To enable the development mode, which **allows you to connect via USB FTDI port**, press the **user button** while powering the MCU. The green LEDs will light up.
