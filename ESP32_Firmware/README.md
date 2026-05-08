# ThermoNoOC: ESP32 Firmware for Intelligent Incubation Control

**Version**: 1.0  
**Project**: TFG - Thermo-Microfluidic Incubator with Flow Control  
**Platform**: ESP32 (Arduino Framework)
**Build System**: PlatformIO

This repository contains the complete, production-ready firmware for an ESP32-based biomedical incubator system. The firmware provides real-time environmental monitoring, temperature regulation, UV lighting control, and microfluidic pump management through an intuitive WiFi interface.

---

## 🎯 System Overview

**ThermoNoOC** is an integrated bioreactor control system that manages:

- **Temperature Control**: Proportional heating via ITO glass heater (37°C setpoint)
- **Environmental Monitoring**: Dual temperature/humidity sensors + UV index + CO₂ tracking
- **UV Lighting**: 4 independent LED arrays with programmable intensity
- **Microfluidic Pumps**: 4 parallel micropumps with 2 flow rate sensors
- **Thermal Management**: PCB temperature monitoring with automatic fan speed control
- **WiFi Interface**: Real-time bidirectional communication with UI (Python/LabVIEW)

---

---

## 📂 Project Architecture

```
ESP32_Firmware/
├── include/
│   └── Pinout.h                    # Global pin definitions (all hardware pins in one place)
├── lib/
│   ├── Control/                    # PCB thermal management
│   │   ├── Control.h
│   │   └── Control.cpp
│   ├── Incubator/                  # Environmental control & sensing
│   │   ├── Incubator.h
│   │   └── Incubator.cpp
│   ├── LED_Array/                  # UV LED lighting (4 channels)
│   │   ├── LED_Array.h
│   │   └── LED_Array.cpp
│   ├── Microfluidics/              # Pump & flow sensor control
│   │   ├── Microfluidics.h
│   │   └── Microfluidics.cpp
│   └── Communication/              # WiFi AP & JSON protocol
│       ├── WifiCommunication.h
│       └── WifiCommunication.cpp
├── src/
│   └── main.cpp                    # Real-time control loop & UI orchestration
├── test/
│   ├── test_i2c_scanner.cpp        # I2C device discovery
│   ├── test_control_ntc_fans.cpp   # Temperature & cooling validation
│   ├── test_led_array.cpp          # UV LED channel verification
│   ├── test_environmental_sensors.cpp
│   ├── test_pumps_logic.cpp        # Pump stress testing
│   ├── test_ito_glass.cpp          # Heater validation
│   ├── test_main.cpp               # System integration test
│   └── test_wifi_communication.cpp # Network communication test
├── platformio.ini                  # Build configuration & dependencies
└── README.md                       # This file
```

---

## 🚀 Main Control System (`main.cpp`)

The main firmware implements a **multitasking control loop** with event-driven architecture running at specific time intervals:

### Control Loop Timing

```
┌─────────────────────────────────────────────────┐
│ Real-time Control Loop                          │
├─────────────────────────────────────────────────┤
│ Event 1: Sensor Reading       (every 100 ms)    │
│ ├─ Read SHT35 temp/humidity sensors             │
│ ├─ Read LTR390 UV index                         │
│ ├─ Read CO₂ sensor (Serial2)                    │
│ ├─ Read PCB temperature (NTC thermistor)        │
│ └─ Update fan speed (thermal management)        │
│                                                 │
│ Event 2: Actuator Control     (every 100 ms)    │
│ ├─ Temperature regulation (ITO heater)          │
│ ├─ UV LED brightness updates                    │
│ └─ Pump frequency & voltage adjustments         │
│                                                 │
│ Event 3: Telemetry Broadcasting (every 500 ms)  │
│ └─ Send sensor data to connected UI clients     │
│                                                 │
│ Event 4: WiFi Command Processing (every 100 ms) │
│ ├─ Listen for UI commands                       │
│ ├─ Parse JSON requests                          │
│ └─ Execute control actions                      │
└─────────────────────────────────────────────────┘
```

### Control Algorithms

#### Temperature Control (Incubator)

- **Algorithm**: Proportional (P) controller
- **Gain**: Kp = 10
- **Output**: 0-255 PWM duty cycle
- **Formula**: `Power = min(max(targetTemp - avgTemp) × 10, 255), 0)`

#### Fan Speed Control (PCB Thermal Management)

- **Hysteresis thresholds**:
  - T > 45°C: 100% speed (255)
  - T > 37.5°C: 75% speed (192)
  - T > 30°C: 50% speed (128)
  - T ≤ 30°C: 25% speed (64)

#### Pump Flow Control

- **Continuous Mode**: Pump runs at constant frequency
- **Pulsed Mode**: Pump cycles with configurable feeding/pausing times
- **Frequency Mapping**: `freq_Hz = flow_mL/min × 100`

---

## 🌐 WiFi Communication Protocol

### Network Configuration

| Parameter              | Value             |
| ---------------------- | ----------------- |
| **Mode**               | Access Point (AP) |
| **SSID**               | `ThermoNoOC`      |
| **Password**           | `thermonooc`      |
| **IP Address**         | 192.168.0.132     |
| **Gateway**            | 192.168.0.1       |
| **Subnet Mask**        | 255.255.255.0     |
| **Port**               | 5000 (TCP)        |
| **Baud Rate** (Serial) | 115200            |

### JSON Command Protocol

All commands are JSON-formatted and transmitted over TCP port 5000.

#### 1. Get Sensor Data

**Request:**

```json
{ "command": "get_sensor_data" }
```

**Response:**

```json
{
  "temp1": 37.25,
  "hum1": 65.4,
  "temp2": 37.18,
  "hum2": 64.9,
  "uv": 125.6,
  "co2": 412,
  "flow1": 0.85,
  "flow2": 0.92
}
```

#### 2. Set Target Temperature

**Request:**

```json
{ "command": "set_temp", "value": "37.5" }
```

**Response:**

```json
{ "status": "ok", "message": "Temperature set" }
```

#### 3. Toggle UV LED Group

**Request:**

```json
{ "command": "uv_toggle", "group": "1", "enabled": "true" }
```

**Response:**

```json
{ "status": "ok", "message": "UV toggled" }
```

#### 4. Set UV Intensity

**Request:**

```json
{ "command": "uv_intensity", "group": "1", "intensity": "200" }
```

**Response:**

```json
{ "status": "ok", "message": "Intensity updated" }
```

**Parameters:**

- `group`: 1-4 (4 independent UV LED arrays)
- `intensity`: 0-255 (PWM duty cycle)

#### 5. Configure Pump Flow & Mode

**Request (Continuous Mode):**

```json
{
  "command": "set_pump_flow_and_mode",
  "pump": "1",
  "flow": "1.5",
  "mode": "continuous",
  "feeding_time": "0",
  "pause_time": "0",
  "cycles": "0"
}
```

**Request (Pulsed Mode):**

```json
{
  "command": "set_pump_flow_and_mode",
  "pump": "1",
  "flow": "2.0",
  "mode": "pulsed",
  "feeding_time": "5.0",
  "pause_time": "10.0",
  "cycles": "10"
}
```

**Parameters:**

- `pump`: 1-2 (2 independent pumps)
- `flow`: Flow rate in mL/min
- `mode`: `"continuous"` or `"pulsed"`
- `feeding_time`: Duration to pump (seconds, pulsed mode only)
- `pause_time`: Duration to pause (seconds, pulsed mode only)
- `cycles`: Number of feed/pause cycles (pulsed mode only)

**Response:**

```json
{ "status": "ok", "message": "Pump configured" }
```

#### 6. Get System Status

**Request:**

```json
{ "command": "get_system_status" }
```

**Response:**

```json
{
  "status": "ok",
  "pcb_temp": 32.5,
  "target_temp": 37.0,
  "pump1_flow": 0.85,
  "pump2_flow": 0.92
}
```

#### 7. Emergency Stop

**Request:**

```json
{ "command": "stop_all" }
```

**Response:**

```json
{ "status": "ok", "message": "System stopped" }
```

**Effects:**

- Temperature set to 25°C
- ITO heater OFF
- UV LEDs OFF
- All pumps OFF
- Fans OFF

---

## 🔧 Hardware Drivers & Specifications

### 1. Control Board (`Control` class)

- **Sensor**: NTC Thermistor (B57164K103J, 10kΩ @ 25°C)
- **Beta Coefficient**: 4300 K
- **ADC Resolution**: 12-bit (0-4095)
- **Fan Control**: PWM Channel 4, 25 kHz, 8-bit
- **Temperature Calculation**: Beta equation with 15-sample averaging

### 2. Environmental Sensors (`Incubator` class)

| Sensor                | Quantity | Protocol | Address | Channel        |
| --------------------- | -------- | -------- | ------- | -------------- |
| SHT35 (Temp/Humidity) | 2        | I2C      | 0x44    | 0x70:CH2,3     |
| LTR390 (UV)           | 1        | I2C      | 0x53    | 0x70:CH4       |
| T6615 (CO₂)           | 1        | UART     | -       | Serial2 (9600) |

### 3. UV LED Array (`LED_Array` class)

- **Channels**: 4 independent PWM
- **Frequency**: 5 kHz (anti-flicker)
- **Resolution**: 8-bit (0-255)
- **GPIO Pins**: 18, 19, 25, 26

### 4. Microfluidics Control (`Microfluidics` class)

**mp-Lowdriver Specifications:**

- **I2C Address**: 0x59
- **Multiplexer**: 0x71 (channels 2-5 for pumps, 6-7 for flow sensors)
- **Voltage Range**: 0-150 Vpp (255 steps)
- **Frequency Range**: 0-800 Hz
- **Frequency Resolution**: 7.8125 Hz/step
- **Formula**: `freqReg = frequency / 7.8125`

**Flow Sensors (SLF3S-0600F):**

- **I2C Address**: 0x08
- **Scale Factor**: 500 counts per mL/min
- **Output**: 16-bit signed integer

### 5. WiFi Interface (`WifiCommunication` class)

- **Mode**: ESP32 SoftAP (Access Point)
- **Protocol**: TCP/IP on port 5000
- **JSON Parser**: Custom lightweight implementation
- **Transmission**: Line-terminated (newline-delimited)

---

## 🛠 I2C Architecture

The system uses **two separate I2C multiplexers** to manage multiple devices:

**Multiplexer 0x70 (Sensors):**

```
I2C_SDA (GPIO21) ─┬─ MUX 0x70
I2C_SCL (GPIO22) ─┤   ├─ CH2: SHT35 #1
                  │   ├─ CH3: SHT35 #2
                  │   ├─ CH4: LTR390 (UV)
                  │   └─ CH5-7: Reserved
```

**Multiplexer 0x71 (Microfluidics):**

```
I2C_SDA (GPIO21) ─┬─ MUX 0x71
I2C_SCL (GPIO22) ─┤   ├─ CH2-5: mp-Lowdrivers (Pumps 1-4)
                  │   ├─ CH6-7: Flow Sensors
```

---

## 🧪 Hardware Validation Tests

To ensure correct PCB assembly and soldering, comprehensive tests are available in `test/`:

| Test                      | File                             | Purpose                                                        |
| ------------------------- | -------------------------------- | -------------------------------------------------------------- |
| **I2C Scanner**           | `test_i2c_scanner.cpp`           | Scan both multiplexers and verify all I2C devices are detected |
| **Control & Fans**        | `test_control_ntc_fans.cpp`      | Validate NTC sensor + fan PWM at different speeds              |
| **LED Array**             | `test_led_array.cpp`             | Test each UV LED channel independently                         |
| **Environmental Sensors** | `test_environmental_sensors.cpp` | Read SHT35, LTR390, T6615 sensors                              |
| **Pumps**                 | `test_pumps_logic.cpp`           | Stress test all 4 pumps at different frequencies               |
| **ITO Heater**            | `test_ito_glass.cpp`             | Validate ITO heater PWM control                                |
| **WiFi Communication**    | `test_wifi_communication.cpp`    | Test WiFi AP and JSON protocol                                 |
| **System Integration**    | `test_main.cpp`                  | Full system test with all modules                              |

**How to run tests:**

1. Copy desired test file content to `src/main.cpp`
2. Build: `PlatformIO: Build` (Ctrl+Alt+B)
3. Upload: `PlatformIO: Upload` (Ctrl+Alt+U)
4. Monitor: `PlatformIO: Serial Monitor` (Ctrl+Alt+M) @ 115200 baud

---

## 💾 Build & Deployment

### Prerequisites

- **Visual Studio Code** with **PlatformIO IDE** extension
- **Python 3.7+** (for PlatformIO CLI)
- **ESP32-DEVKIT-V1** board

### Build Configuration

The `platformio.ini` file contains:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    sensirion/arduino-sht @ ^1.2.5
    adafruit/Adafruit LTR390 Library @ ^1.1.2

build_src_filter =
    +<*>
    -<../test/>  ; Exclude test folder for main firmware
```

### Build & Upload Steps

1. **Open project in VS Code** with PlatformIO
2. **Build**:
   ```bash
   platformio run
   ```
3. **Upload**:
   ```bash
   platformio run --target upload
   ```
4. **Monitor serial output**:
   ```bash
   platformio device monitor --baud 115200
   ```

### Firmware Flash Layout

| Section         | Size  | Address  |
| --------------- | ----- | -------- |
| Bootloader      | 16 KB | 0x1000   |
| Partition Table | 4 KB  | 0x8000   |
| Factory App     | 1 MB  | 0x10000  |
| OTA Partition   | 1 MB  | 0x110000 |
| SPIFFS          | 2 MB  | 0x210000 |

---

## 📊 Expected Startup Sequence

When the system boots, the serial monitor shows:

```
=== ThermoNoOC ESP32 Firmware ===
Initializing system...
✓ Control board initialized
✓ Incubator initialized
✓ WiFi AP started

System Ready: AP 'ThermoNoOC' started.
IP: 192.168.0.132
Port: 5000

Target Temperature set to: 37.00 °C
UV Group 1: ON
Pump 1 config: flow=1.50 mL/min, mode=continuous
...
```

---

## 🔐 Safety Features

1. **Emergency Stop Command**: `{"command":"stop_all"}` immediately disables all actuators
2. **Temperature Limits**: Maximum ITO power is 255 (150 Vpp), enforced via `constrain()`
3. **Pump Voltage Limits**: 0-255 range with frequency bounds (50-300 Hz)
4. **Fan Hysteresis**: Prevents rapid on/off cycling with temperature thresholds
5. **I2C Error Handling**: Graceful fallback if sensors are unavailable
6. **Watchdog Protection**: [Optional] Can add hardware watchdog timer

---

## 🐛 Troubleshooting

### System doesn't start

1. **Check serial monitor** at 115200 baud for startup messages
2. **Verify I2C connections**: Run `test_i2c_scanner.cpp`
3. **Check power supply**: Ensure 5V/GND connections are secure

### Sensors not reading

1. **I2C Multiplexer issue**: Verify MUX addresses (0x70, 0x71) with scanner test
2. **Device disconnection**: Check cable continuity
3. **Wrong channel**: Verify channel assignments in `Pinout.h`

### Pumps not responding

1. **Check mp-Lowdriver address**: Should be 0x59 on I2C bus
2. **Verify frequency range**: 50-300 Hz for BP7 pumps
3. **Check power delivery**: Use oscilloscope to verify voltage output

### WiFi connection issues

1. **Verify SSID/password**: Check `WifiCommunication.cpp` initialization
2. **Check TCP port**: Port 5000 must be accessible
3. **Monitor serial output**: Look for "AP started" message

### Temperature control instability

1. **Tune Kp gain**: Adjust in `Incubator::updateActuators()` (currently 10)
2. **Check sensor readings**: Verify SHT35 values via serial monitor
3. **Verify heater output**: Use `test_ito_glass.cpp` to test ITO heater

---

## 📋 Development Notes

### Code Standards

- **Naming**: `camelCase` for variables/functions, `UPPER_CASE` for constants
- **Comments**: Every class and public method has detailed documentation
- **Modularity**: Each hardware component has its own library class
- **Error Handling**: Graceful degradation if non-critical systems fail

### Adding New Features

To add a new hardware component:

1. Create `lib/YourComponent/YourComponent.h` with class definition
2. Implement `lib/YourComponent/YourComponent.cpp`
3. Add `#include "YourComponent.h"` to relevant files
4. Instantiate in `main.cpp` and integrate into control loop
5. Create test file in `test/test_yourcomponent.cpp`

---

## 📝 API Reference Summary

### Core Classes

#### `Control` - PCB Thermal Management

```cpp
void begin();                    // Initialize PWM and ADC
float getPCBTemperature();       // Read NTC thermistor
void setFansSpeed(uint8_t speed); // Set fan PWM (0-255)
```

#### `Incubator` - Environmental Control

```cpp
void begin();                      // Initialize all sensors
void readEnvironment();             // Poll all sensors
void updateActuators();             // Apply control algorithms
void setITOPower(uint8_t power);   // Control heater PWM
```

#### `LED_Array` - UV Lighting

```cpp
void begin();                    // Initialize LED PWM
void setBrightness(int group, int value); // Set LED intensity
void allOff();                   // Turn off all LEDs
```

#### `Microfluidics` - Pump & Flow Control

```cpp
void begin();                    // Initialize pumps & sensors
float getFlowRate(int sensorNum); // Read flow rate (mL/min)
void setPumpVoltage(int pumpNum, uint8_t voltage); // Amplitude
void setPumpFrequency(int pumpNum, uint16_t frequency); // Frequency
```

#### `WifiCommunication` - WiFi Interface

```cpp
void begin(IPAddress ip, IPAddress gw, IPAddress sn); // Start AP
String getRequest(WiFiClient &client); // Read incoming data
String extractJsonValue(String data, String key);     // Parse JSON
String buildSensorJson(...);    // Build response packet
```

---

## 📚 References

- **ESP32 Datasheet**: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
- **Arduino SHT35 Library**: https://github.com/Sensirion/arduino-sht
- **Adafruit LTR390**: https://github.com/adafruit/Adafruit_LTR390
- **PlatformIO Docs**: https://docs.platformio.org/
- **I2C Protocol**: https://www.nxp.com/docs/en/user-manual/UM10204.pdf

---

## 👤 Author & Support

**Project**: TFG - Thermo-Microfluidic Incubator (ThermoNoOC)  
**Institution**: Universitat de Barcelona  
**Status**: Production Firmware v1.0

For issues, feature requests, or technical support, please contact the development team or check the project documentation.

---

**Last Updated**: May 2026
