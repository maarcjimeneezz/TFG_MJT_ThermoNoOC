# TFG_MJT_ThermoNoOC
This repository contains the source code and software architecture for ThermoNoOC, a Bachelor’s Degree Thesis (TFG) project.
- **Author**: Marc Jiménez Torra
- **Institution**: University of Barcelona (UB)
- **Collaboration**: Department of Electronic and Biomedical Engineering & D2IN Research Group.

# 📝 Project Description
The software component of **ThermoNoOC** is designed to manage and automate a controlled environment for the dynamic culture of organoids. Its primary function is to handle real-time sensor data acquisition and execute control loops (such as PID) to maintain optimal biological conditions.

# 🛠️ Technical Implementation
This codebase covers the following areas:
- **Environmental Monitoring**: Real-time processing of temperature, CO2, UV and humidity sensors.
- **Control Algorithms**: Implementation of feedback loops for precise environmental stabilization.
- **Dynamic Flow Management**: Logic for controlling micropumps and flow sensors to simulate dynamic culture conditions.
- **Data Logging**: Systematic recording of experimental parameters for posterior analysis.

# 🌐 System Architecture

## Hardware
- **Microcontroller**: ESP32 (runs firmware for real-time control)
- **Communication**: WiFi Access Point mode (ThermoNoOC network)
- **Sensors**: Temperature, humidity, UV, CO₂, flow rate sensors
- **Actuators**: Heating element, UV LEDs (4 groups), micropumps (2 circuits), cooling fans

## Software Components

### 1. ESP32 Firmware
- **Location**: `ESP32_Firmware/`
- **Platform**: Arduino framework with PlatformIO
- **Functions**: Real-time sensor reading, control algorithms, WiFi communication
- **Network**: Acts as WiFi Access Point (AP) for UI communication

### 2. User Interface (UI)
- **Location**: `UI/`
- **Platform**: Python (customtkinter, matplotlib)
- **Functions**: Real-time monitoring, control commands, data visualization and export
- **Connection**: TCP/IP over WiFi to ESP32 (192.168.0.132:5000)

## WiFi Configuration
| Parameter | Value |
|-----------|-------|
| SSID | ThermoNoOC |
| Password | thermonooc |
| IP Address | 192.168.0.132 |
| Port | 5000 |

# 🚀 Quick Start

### Setup ESP32 Firmware
```bash
cd ESP32_Firmware
platformio run -t upload
```

### Setup Python UI
```bash
cd UI
pip install -r requirements.txt
python User_interface.py
```

### Connect
1. Connect to WiFi network: **ThermoNoOC** (password: **thermonooc**)
2. Click "Connect ESP32" in the UI
3. System is ready to use

# 📋 Features

- **Real-time Monitoring**: Temperature, humidity, UV, CO₂ and flow sensors
- **Precise Temperature Control**: PID-based heating regulation
- **UV LED Control**: 4 independent LED arrays with intensity adjustment (0-255 PWM)
- **Microfluidics Control**: 2 independent pump circuits with continuous/pulsed modes
- **Thermal Management**: Automatic fan speed control based on PCB temperature
- **Data Export**: CSV export of all sensor readings with timestamps
- **Safety Features**: Emergency stop command, system status monitoring

# 🤝 Acknowledgments
Special thanks to the D2IN Research Group and the Department of Electronic and Biomedical Engineering (UB) for their guidance and for providing the laboratory framework for this development.
