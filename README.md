# FirstArduinoProject

A PlatformIO project for the **Arduino Uno R4 WiFi** that monitors analog voltage, controls a servo motor, displays data on the built-in LED matrix, and hosts a real-time web dashboard over WiFi.

## Hardware

| Component | Pin |
|-----------|-----|
| Arduino Uno R4 WiFi | — |
| Servo motor | D9 |
| Red LED | D2 |
| Analog voltage input | A0 |
| Built-in 8×12 LED matrix | — |
| WiFi (WiFiS3) | — |

## Features

- **Voltage reading** — reads 0–4.66 V from pin A0 every 500 ms (10-bit ADC)
- **Servo control** — maps voltage linearly to 0–180°; resets to 0° every 3 seconds then returns to the voltage-based angle
- **LED matrix** — scrolls the current voltage value (e.g. `3.45V`) on the built-in matrix in real time
- **Status LED** — blinks the red LED at ~6 Hz as a heartbeat indicator
- **Web dashboard** — serves an auto-refreshing HTML page (port 80) showing live voltage and servo angle

## Web Dashboard

Once connected, open a browser to the board's IP address. The page refreshes every second and displays:

```
Voltage: 2.31 V
Servo:   89°
```

The IP address is printed to the serial monitor on startup.

## Setup

1. Copy [include/secrets.h](include/secrets.h) and fill in your WiFi credentials:
   ```cpp
   #define SECRET_SSID "your_network"
   #define SECRET_PASS "your_password"
   ```
2. Open the project in VS Code with the PlatformIO extension.
3. Build and upload: `PlatformIO: Upload` (or `pio run -t upload`).
4. Open the serial monitor at **115200 baud** to see the assigned IP address and live readings.

## Dependencies

Declared in [platformio.ini](platformio.ini):

| Library | Version |
|---------|---------|
| ArduinoGraphics | ^1.0.0 |
| Servo | ^1.2.0 |
| WiFiS3 | bundled with board |
| Arduino_LED_Matrix | bundled with board |

## Project Structure

```
FirstArduinoProject/
├── platformio.ini   # Board, platform, and library configuration
├── src/
│   └── main.cpp     # Application logic
├── include/
│   └── secrets.h    # WiFi credentials (not committed)
├── lib/             # Reserved for local libraries
└── test/            # Reserved for unit tests
```

## Serial Output

The serial monitor (115200 baud) logs:

- WiFi firmware version and connection status
- RSSI signal strength and assigned IP address
- Voltage and servo angle every 500 ms
- Web server request notifications
- Servo reset/restore events
# FirstArduinoProject
# ArduinoServoCtrl
# ArduinoServoCtrl
