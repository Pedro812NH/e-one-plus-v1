# ESP32 Power Monitoring System

A firmware for ESP32 NodeMCU that reads data from an SCT-013-000 current sensor, calculates power metrics, and transmits data to a local backend.

## Features

- Real-time monitoring of electrical current using SCT-013-000 sensor
- Calculation of current, power, and cumulative energy consumption
- Wi-Fi configuration via captive portal
- Automatic data transmission to local backend via HTTP POST
- Connection loss handling with retry mechanism and data buffering
- Extensible design for future features like OTA updates and local AI

## Hardware Requirements

- ESP32 NodeMCU development board
- SCT-013-000 current transformer (100A max, voltage output type)
- 33Ω burden resistor
- 10μF capacitor
- Breadboard and jumper wires

## Hardware Setup

1. Connect the burden resistor (33Ω) across the output terminals of the SCT-013-000
2. Connect one end of the SCT-013-000 output to the ESP32 GND
3. Connect the other end to GPIO34 (ADC pin)
4. Connect a 10μF capacitor between AREF and GND for stabilization

## Software Setup

### Prerequisites

1. Install [PlatformIO](https://platformio.org/install) (recommended) or Arduino IDE
2. Clone this repository or download the source code

### Building and Uploading

This repository contains multiple firmware applications. You can choose which one to build using PlatformIO environments.

#### Available Firmware Environments

*   `main_app` (Default): The main, feature-rich application with WiFiManager, OTA updates, and AI features.
*   `tft_app`: A version that displays power metrics on a TFT screen.
*   `minimal_app`: A simplified version with hardcoded WiFi credentials for quick testing.
*   `simple_test`: A basic test sketch to ensure your toolchain is working.

#### Using PlatformIO

1.  Open the project in PlatformIO.
2.  To build a specific firmware, use the `-e` flag with `platformio run`. For example, to build the main application:
    ```bash
    platformio run -e main_app
    ```
    If you don't specify an environment, the `main_app` will be built by default.

3.  To upload a specific firmware, use the `-e` flag with `platformio upload`:
    ```bash
    platformio upload -e main_app
    ```

4.  To upload the file system image (for configurations), run:
    ```bash
    platformio run --target uploadfs -e main_app
    ```

#### Using Arduino IDE

1.  Install the ESP32 board support package.
2.  Install the required libraries for the specific firmware you want to use (see `platformio.ini` for details).
3.  Open the desired `.cpp` file (e.g., `main.cpp`) in the Arduino IDE. You will be prompted to create a sketch folder.
4.  Copy all required header files (`.h`) and implementation files (`.cpp`) for your chosen application into the new sketch folder.
5.  Select the correct board and port.
6.  Upload the sketch.

## First-time Setup

1. Power on the ESP32
2. Connect to the "ESP32_Power_Monitor" WiFi network from your phone/computer
3. A captive portal will open automatically (or navigate to 192.168.4.1)
4. Enter your WiFi credentials
5. Configure the backend URL if needed
6. The device will reboot and connect to your WiFi network

## Configuration

The system can be reconfigured at any time by:

1. Wait until the device is connected to WiFi
2. Manually trigger the configuration portal by power cycling the device 3 times quickly
3. Connect to the "ESP32_Power_Monitor" network again
4. Make your changes in the captive portal

## Backend Integration

The system sends data to the specified backend URL using HTTP POST requests with JSON payload:

```json
{
  "timestamp": 1234567890,
  "current_amps": 2.5,
  "voltage_volts": 230.0,
  "power_watts": 575.0,
  "energy_kwh": 1.25,
  "device_id": "ESP32_Power_Monitor"
}
