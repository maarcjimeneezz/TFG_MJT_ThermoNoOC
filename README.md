# ThermoNoOC

**Author**: Marc Jiménez Torra  
**Institution**: Universitat de Barcelona (UB)  
**Collaboration**: Department of Electronic and Biomedical Engineering & D2IN Research Group

---

## Project Description

**ThermoNoOC** is a controlled environment system for the dynamic culture of organoids. It automates real-time sensor acquisition and closed-loop control to maintain optimal biological conditions, and provides a desktop interface for monitoring, configuration, and data logging.

---

## System Architecture

```
┌──────────────────────────────────────────────────────┐
│               Desktop PC (Python UI)                 │
│         user-interface-v2.py  ·  WebSocket client    │
└─────────────────────┬────────────────────────────────┘
                      │  WiFi  ·  ws://192.168.4.1:5000
┌─────────────────────┴────────────────────────────────┐
│              ESP32 (Access Point)                    │
│  main.cpp  ·  Incubator  ·  LED_Array                │
│  Microfluidics  ·  Control  ·  WiFiManager           │
└──────────────────────────────────────────────────────┘
```

### Hardware

| Component | Description |
|-----------|-------------|
| Microcontroller | ESP32-DEVKIT-V1 |
| Incubator sensors | 2× SHT35 (temp/humidity), LTR390 (UV), T6615 (CO₂) |
| Actuators | ITO glass heater, 4× UV LED groups, 4× micropumps |
| Flow sensors | 2× Sensirion SLF3S-0600F (0–2000 µL/min) |
| Thermal management | NTC thermistor + PWM fan |

### Software Components

| Component | Location | Description |
|-----------|----------|-------------|
| ESP32 Firmware | `ESP32_Firmware/` | Real-time control, sensor reading, WebSocket server |
| Desktop UI | `UI/user-interface-v2.py` | Monitoring, configuration, CSV data logging |

---

## WiFi Configuration

| Parameter | Value |
|-----------|-------|
| SSID | `ThermoNoOC` |
| Password | `thermonooc` |
| ESP32 IP | `192.168.4.1` |
| WebSocket port | `5000` |

---

## Quick Start

### 1. Flash the firmware

```bash
cd ESP32_Firmware
platformio run --target upload
```

### 2. Install UI dependencies

```bash
cd UI
pip install customtkinter matplotlib websocket-client
```

### 3. Run the UI

```bash
python user-interface-v2.py
```

### 4. Connect

1. Join the **ThermoNoOC** WiFi network (password: `thermonooc`)
2. Click **Connect** in the UI header
3. Toggle **Incubator Closed** to enable sensor/actuator operation

---

## Features

- **Incubator tab** — Temperature (dual sensor), humidity, CO₂ monitoring with live plots; PID-controlled ITO glass heater with configurable setpoint
- **UV Light tab** — Live irradiance plot; 4 independent UV LED groups with enable/intensity control
- **Microfluidics tab** — Flow rate monitoring (2 circuits); PID-controlled pump flow; continuous and pulsed feeding modes with configurable parameters; automatic bubble-removal pump activation
- **Safety interlocks** — Incubator interlock (gates all actuators) and microfluidics interlock (gates pumps independently); both visible and controllable from every relevant tab
- **Data logging** — Automatic CSV export to `UI/Data_Logging/YYYYMMDD/` while the incubator interlock is closed
- **Theme toggle** — Dark / Light mode

---

## Repository Structure

```
TFG_MJT_ThermoNoOC/
├── ESP32_Firmware/          # PlatformIO firmware project
│   ├── include/Pinout.h     # All GPIO and I2C address definitions
│   ├── lib/                 # Domain modules (one class per concern)
│   │   ├── Control/         # PCB thermal management (NTC + fan)
│   │   ├── Incubator/       # Environmental sensing and ITO PID
│   │   ├── LED_Array/       # UV LED PWM control
│   │   ├── Microfluidics/   # Pump PID, flow sensors, circuit logic
│   │   └── WiFi/            # WebSocket Access Point server
│   ├── src/main.cpp         # Control loop and command dispatcher
│   ├── test/                # Per-module hardware validation tests
│   └── README.md            # Firmware documentation
├── UI/
│   ├── user-interface-v2.py # Desktop control panel
│   ├── Data_Logging/        # Auto-generated CSV session files
│   └── README.md            # UI documentation
└── README.md                # This file
```

---

## Acknowledgments

Special thanks to the D2IN Research Group and the Department of Electronic and Biomedical Engineering (UB) for their guidance and laboratory framework.
