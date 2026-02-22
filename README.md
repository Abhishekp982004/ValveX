# ValveX — Smart Water Intelligence System

[![Language](https://img.shields.io/badge/language-C%2B%2B%20%2F%20JavaScript-blue)](#)
[![Platform](https://img.shields.io/badge/platform-ESP32%20%2B%20MQTT%20%2B%20Node--RED-informational)](#)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE)

ValveX is an **IoT-based smart water monitoring and leak-prevention system** that measures water flow in real time, detects abnormal usage (e.g., continuous flow/leaks), and can **automatically control a valve and pump**. It’s designed to keep safety/automation at the edge (ESP32) while enabling remote monitoring and manual override through an MQTT-connected UI/middleware.

## What the project does

- Reads flow pulses from a water flow sensor and computes **flow rate** on an **ESP32**.
- Publishes live telemetry to **MQTT** (e.g., `water/flow`).
- Subscribes to an MQTT control topic (e.g., `water/valve`) for **manual remote override**.
- Controls **valve + pump** through a relay and motor driver logic.
- Displays device state on an **SSD1306 OLED** display.

## Why the project is useful

Water leakage and continuous unnoticed flow waste water and can damage infrastructure. ValveX helps by:

- Providing **real-time visibility** into flow rate.
- Enabling **fast responses** (local control on ESP32).
- Supporting **remote manual control** via MQTT (works well with Node-RED dashboards).
- Offering a clean base to extend into: burst detection, night anomaly detection, alerts, and dashboards.

## Repository structure

> Note: This is based on the current repository layout observed (`firmware/`, `middleware/`, `docs/`). If you add more modules (dashboard screenshots, schematics, etc.), update this tree.

```text
.
├── firmware/
│   └── esp32/
│       └── valveX_esp32.ino     # ESP32 firmware (Arduino)
├── middleware/                   # Middleware / UI integration (e.g., Node-RED, scripts)
├── docs/                         # Project documentation
├── LICENSE
└── README.md
```

## Getting started

### Prerequisites

#### Hardware (typical setup)
- ESP32 development board
- Water flow sensor (pulse output)
- Solenoid valve (normally closed recommended) + relay module
- Pump + motor driver (or relay, depending on your design)
- SSD1306 OLED (I2C)
- Appropriate power supply + buck converter (as required by your hardware)

#### Software
- Arduino IDE **or** PlatformIO for ESP32 development
- MQTT broker (e.g., Mosquitto)
- (Optional) Node-RED for dashboards and easy MQTT wiring

---

## Setup

### 1) MQTT broker (example: Mosquitto)

**macOS (Homebrew):**
```bash
brew install mosquitto
mosquitto -v
```

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y mosquitto
mosquitto -v
```

### 2) ESP32 firmware

Open the Arduino sketch:

- `firmware/esp32/valveX_esp32.ino`

Update these values in the sketch to match your environment:
- Wi‑Fi SSID/password
- MQTT broker IP/host + port

Upload to the ESP32.

### 3) MQTT topics

ValveX uses MQTT to publish telemetry and receive control commands.

| Topic | Direction | Purpose |
|------|-----------|---------|
| `water/flow` | Publish | Flow telemetry (e.g., L/min) |
| `water/valve` | Subscribe | Manual valve control (`"1"` open, other/`"0"` close) |

**Manual control example:**
```bash
# Open valve (and start pump)
mosquitto_pub -h <MQTT_HOST> -t water/valve -m "1"

# Close valve (and stop pump)
mosquitto_pub -h <MQTT_HOST> -t water/valve -m "0"
```

**Listen to flow telemetry:**
```bash
mosquitto_sub -h <MQTT_HOST> -t water/flow -v
```

## Usage overview

1. Power the ESP32 + connected hardware.
2. Start your MQTT broker.
3. Observe `water/flow` telemetry.
4. Use `water/valve` to manually open/close the valve remotely.
5. (Optional) Connect Node-RED to visualize telemetry and build a dashboard.

## Configuration notes (firmware)

The ESP32 sketch defines pin mappings and control logic (pump + valve). Review and adapt to your wiring:

- Flow sensor input pin
- Relay pin for valve
- Motor driver pins / PWM channel for pump

**Important:** Ensure your relay logic level matches your relay module and wiring (active HIGH vs active LOW).

## Where users can get help

- Repository docs: `docs/`
- Issues: open a GitHub Issue in this repository with:
  - your board type (ESP32 variant),
  - wiring diagram/photo,
  - MQTT broker used,
  - logs (Serial output),
  - what you expected vs what happened.

## Who maintains and contributes

**Maintainer:** @Abhishekp982004

Contributions are welcome:
- Bug fixes (connectivity, reconnection logic, stability)
- Improvements to MQTT payload formats
- Node-RED flows and dashboards
- Leak/anomaly detection logic and test data

Please see (or add) contribution guidelines:
- `docs/CONTRIBUTING.md` (recommended)
- If you don’t have one yet, create it and include: how to run/flash, coding style, and how to propose changes.

## License

MIT — see [LICENSE](LICENSE).
