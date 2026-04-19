# ESP32-C6 Asymmetric Multiprocessing (AMP): LP Core Interrupt Watcher

## Overview
This repository demonstrates an enterprise-grade Asymmetric Multiprocessing (AMP) architecture on the **ESP32-C6**. Instead of relying on standard deep-sleep hardware timers, this project actively utilizes the **Low-Power (LP) RISC-V Coprocessor** to continuously monitor sensors while the Main CPU is completely powered down.

This architecture is heavily used in commercial ultra-low-power devices because it allows for complex logic, continuous polling, and state management at a fraction of the power cost (~10µA).

## Core Architecture

### 1. The "Night Watchman" (LP Core)
* Runs on a dedicated, low-power 20MHz RISC-V core.
* Compiled independently using bare-metal C (no FreeRTOS).
* Continuously polls a specific LP_IO pin (GPIO 4) and sleeps for 100ms between checks to minimize active duty cycles.
* Triggers an interrupt to wake the Main CPU only when a specific hardware event occurs.

### 2. The "Big Brain" (Main CPU)
* Runs the standard ESP-IDF RTOS environment on the 160MHz RISC-V core.
* Wakes up, evaluates the `esp_sleep_get_wakeup_cause()`, and acts on the alarm.
* Re-loads the LP Core firmware into RTC memory and commits "suicide" (returns to Deep Sleep) to save battery.

### 3. Cross-Compiler Build System
This project utilizes a dual-toolchain CMake configuration. The ESP-IDF automatically halts the main build, compiles the `ulp_main.c` file using the specialized LP Core toolchain, generates an assembly header, and embeds the binary directly into the main `.elf` image via the `ulp_embed_binary()` command.

## Hardware Setup & The "Antenna Effect"
* **Monitoring Pin:** GPIO 4 (LP_IO 4)
* **Wiring:** Connect a physical jumper wire from **GPIO 4 to 3.3V**. 
* **The Trigger:** Pull the wire out of 3.3V and tap it to **GND**.

*Engineering Note:* We use a hardwired pull-up to 3.3V instead of relying on the ESP32's internal ~45kΩ pull-up resistor. Floating pins act as microscopic antennas and easily pick up EMI (electromagnetic interference), causing false wake-ups in the high-sensitivity LP Core domain. A physical pull-up ensures bulletproof stability.

## How to Test the System

Testing an AMP architecture requires simulating real-world physical events. Follow these steps to observe the handoff between processors:

1. **Physical Setup:** Ensure your jumper wire connects GPIO 4 directly to a 3.3V pin on your board.
2. **Build and Flash:** Run `idf.py build flash monitor`.
3. **Arm the System:** Press the physical `RESET` button on the ESP32-C6. 
4. **Observe the Sleep Transition:** Watch the terminal. You will see:
   ```text
   Loading LP Core Firmware...
   Night Watchman (LP Core) is active. Main CPU going to sleep.
*Note:* At this point, the USB connection will drop (Device not configured). This proves the Main CPU is physically powered off. The board is now running entirely on the LP Coprocessor.
5. **Trigger the Alarm**: Pull the jumper wire out of the 3.3V pin and immediately touch it to a GND pin.
6. **Observe the Wake Transition**: The terminal will instantly reconnect and print the alarm message, proving the LP Core successfully woke the Main CPU

## Industry Use Cases: Why build this?

Why write two separate C programs instead of just using a standard Deep Sleep wake stub?

* **Smart Home Security (Door/Window Sensors):** A standard wake pin triggers the Main CPU instantly on any voltage drop. By using the LP Core, you can write logic to filter out "bounces" (e.g., a door rattling in the wind) and only wake the high-power Main CPU if the door stays open for more than 2 seconds.

* **Wearables (Smartwatches):** The LP Core can continuously read an accelerometer via I2C to detect a "wrist raise" gesture while the main screen and CPU are off, allowing a smartwatch to last for days instead of hours.

* **Industrial IoT (Leak & Vibration Detectors):** The LP Core can constantly sample analog water sensors or vibration thresholds in a remote pipeline, only waking up the Main CPU to fire off an MQTT Wi-Fi message if a critical threshold is breached.

## Project Structure
```text
your_project_folder/
├── CMakeLists.txt
└── main/
    ├── CMakeLists.txt          <-- Configured with ulp_embed_binary()
    ├── main.c                  <-- Main CPU (Xtensa/RISC-V) code
    └── ulp/
        └── ulp_main.c          <-- LP Core (RISC-V) bare-metal code

## Build & Run Instructions
1. Enable the LP Core in menuconfig
   ```bash
   idf.py menuconfig
    # Component config -> Ultra Low Power (ULP) Co-processor -> Enable (LP Core RISC-V)

2. Build the firmware and flash it to the board:
   ```bash
   idf.py build flash monitor