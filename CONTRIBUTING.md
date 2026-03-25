# Developer info and tools

The software uses RTOS tasks to manage individual board peripherals.

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
