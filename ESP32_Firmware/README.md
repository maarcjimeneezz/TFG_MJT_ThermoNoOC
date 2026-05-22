# ThermoNoOC — ESP32 Firmware

**Platform**: ESP32-DEVKIT-V1 · Arduino framework · PlatformIO  
**Project**: TFG — Thermo-Microfluidic Incubator (ThermoNoOC)

---

## Overview

The firmware runs on an ESP32 configured as a **WiFi Access Point**. It continuously reads all sensors, runs closed-loop control algorithms, and communicates with the desktop UI over a **WebSocket** connection on port 5000. All domain logic is isolated into independent library modules; `main.cpp` is the sole place where modules are instantiated and wired together.

---

## Project Structure

```
ESP32_Firmware/
├── include/
│   └── Pinout.h                     # All GPIO pins and I2C addresses
├── lib/
│   ├── Control/                     # PCB thermal management
│   │   ├── Control.h
│   │   └── Control.cpp
│   ├── Incubator/                   # Environmental sensors + ITO heater PID
│   │   ├── Incubator.h
│   │   └── Incubator.cpp
│   ├── LED_Array/                   # UV LED groups (4 channels)
│   │   ├── LED_Array.h
│   │   └── LED_Array.cpp
│   ├── Microfluidics/               # Pump PID + flow sensors
│   │   ├── Microfluidics.h
│   │   └── Microfluidics.cpp
│   └── WiFi/                        # WebSocket Access Point server
│       ├── WiFiManager.h
│       └── WiFiManager.cpp
├── src/
│   └── main.cpp                     # Control loop and command dispatcher
├── test/
│   ├── test_i2c_scanner.cpp
│   ├── test_control_ntc_fans.cpp
│   ├── test_led_array.cpp
│   ├── test_environmental_sensors.cpp
│   ├── test_pumps_logic.cpp
│   ├── test_ito_glass.cpp
│   ├── test_wifi_communication.cpp
│   └── test_main.cpp
├── platformio.ini
└── README.md
```

---

## Control Loop (`main.cpp`)

```
loop()
├── wifi.loop()                       // always — handles WS heartbeats and commands
│
├── if (is_Incubator_Closed)          // incubator safety interlock
│   ├── incubator.read_All_Sensors()
│   ├── incubator.update_Heater_PWM()
│   ├── control.update_Fan_Speed(...)
│   ├── leds.update_All_Groups()
│   │
│   └── if (is_Micro_Closed)          // microfluidics safety interlock
│       └── fluidics.update_Pumps()
│
└── every 1 000 ms
    └── wifi.broadcast(telemetry_JSON)
```

Both safety flags are set/cleared by the UI via `SET_INCUBATOR:` and `SET_MICRO:` commands. All actuators are fully inhibited while their respective interlock is open.

---

## WebSocket Protocol

### Network

| Parameter | Value |
|-----------|-------|
| Mode | Access Point (AP) |
| SSID | `ThermoNoOC` |
| Password | `thermonooc` |
| IP | `192.168.4.1` |
| Port | `5000` (WebSocket) |
| Serial baud | `115200` |

### Telemetry (ESP32 → UI, every 1 s)

```json
{
  "temp1": 37.25,  "hum1": 64.8,
  "temp2": 37.18,  "hum2": 65.1,
  "uvIndex": 0.042, "uvW": 0.0011,
  "co2": 0.0410,
  "flow1": 523.4,  "flow2": 0.0
}
```

`flow1` / `flow2` are the last values cached by the flow-sensor PID tick (no extra I2C read).

### Commands (UI → ESP32)

| Command | Format | Description |
|---------|--------|-------------|
| Set temperature | `SET_TEMP:<float>` | Target °C, e.g. `SET_TEMP:37.0` |
| Set LED group | `SET_LED:<group>:<en>:<intensity>` | group 1–4, en 0/1, intensity 0–100 |
| Incubator interlock | `SET_INCUBATOR:<0\|1>` | 1 = closed (enables actuators) |
| Microfluidics interlock | `SET_MICRO:<0\|1>` | 1 = closed (enables pumps); 0 immediately stops all pumps |
| Configure pump circuit | `SET_PUMP:<circuit>:<flow>:<pulsed>:<feed_s>:<pause_s>:<cycles>` | See pump section |

#### `SET_PUMP` fields

| Field | Description |
|-------|-------------|
| `circuit` | 1 or 2 |
| `flow` | Continuous: target flow rate (µL/min). Pulsed: volume per pulse (µL) |
| `pulsed` | 0 = continuous, 1 = pulsed |
| `feed_s` | Feed duration per cycle (s), pulsed mode only |
| `pause_s` | Pause duration per cycle (s), pulsed mode only |
| `cycles` | Number of cycles; 0 = infinite |

---

## Hardware Modules

### I2C Architecture

Two TCA9548A multiplexers share the same SDA/SCL bus (GPIO 21/22):

```
MUX 0x70 — Environmental sensors
  CH 2 → SHT35 #1  (top sensor)
  CH 3 → SHT35 #2  (bottom sensor)
  CH 4 → LTR390    (UV index / irradiance)

MUX 0x71 — Microfluidics
  CH 2 → mp-Lowdriver  Pump 1  (fluid,  circuit 1)
  CH 3 → mp-Lowdriver  Pump 2  (bubble, circuit 1)
  CH 4 → mp-Lowdriver  Pump 3  (fluid,  circuit 2)
  CH 5 → mp-Lowdriver  Pump 4  (bubble, circuit 2)
  CH 6 → SLF3S-0600F  Flow sensor 1  (circuit 1)
  CH 7 → SLF3S-0600F  Flow sensor 2  (circuit 2)
```

### Incubator (`Incubator` class)

| Sensor | Qty | Protocol | I2C address | MUX channel |
|--------|-----|----------|-------------|-------------|
| SHT35 (temp/humidity) | 2 | I2C | 0x44 | 0x70: CH2, CH3 |
| LTR390 (UV) | 1 | I2C | 0x53 | 0x70: CH4 |
| T6615 (CO₂) | 1 | UART2 | — | GPIO 16/17, 9600 baud |

**ITO heater PID** (PWM channel 5, 5 kHz, 8-bit):

| Parameter | Value |
|-----------|-------|
| Kp | 3.0 |
| Ki | 0.05 |
| Kd | 2.0 |
| Integral anti-windup clamp | ±40 |
| Derivative low-pass α | 0.1 |
| Max PWM output | 30 / 255 (thermal protection) |
| Forced cooling | 5 s on → 3 s forced off |
| Setpoint ramp | 0.5 °C/s max |

### UV LED Array (`LED_Array` class)

4 independent PWM channels (GPIO 18, 19, 25, 26), 5 kHz, 8-bit. Each group can be enabled/disabled and have its intensity set from 0–100 % via `SET_LED`.

### Microfluidics (`Microfluidics` class)

**Circuit layout:**

| Circuit | Fluid pump | Bubble-removal pump | Flow sensor |
|---------|-----------|---------------------|-------------|
| 1 | Pump 1 (idx 0) | Pump 2 (idx 1) | Sensor 1 |
| 2 | Pump 3 (idx 2) | Pump 4 (idx 3) | Sensor 2 |

Pumps 1 and 3 control fluid flow and are driven by the PID. Pumps 2 and 4 run at a fixed operating point whenever their circuit's fluid pump is active, and stop when it stops.

**mp-Lowdriver specs:**

| Parameter | Value |
|-----------|-------|
| I2C address | 0x59 (shared; differentiated by MUX channel) |
| Voltage range | 0–150 Vpp (register byte 0–255) |
| Frequency range | 8–800 Hz (firmware-limited) |
| Frequency resolution | 7.8125 Hz / LSB |
| Settle time (STOP → write) | 5 ms |

**Flow sensor (SLF3S-0600F):**

| Parameter | Value |
|-----------|-------|
| I2C address | 0x08 |
| Range | 0–2000 µL/min |
| Raw scale | 500 counts / mL/min |
| Output | 16-bit signed integer + CRC byte |

**Pump PID (per circuit):**

| Parameter | Value |
|-----------|-------|
| Sample interval | 100 ms |
| Feedforward | `flow_Rate_To_Freq_Byte(setpoint)` (open-loop calibration) |
| Kp | 0.003 freqByte / (µL/min) |
| Ki | 0.001 freqByte / (µL/min·s) |
| Anti-windup | Integral frozen when output saturates |
| PID reset | On new `SET_PUMP` config or `SET_MICRO:0` |

**Bubble-removal pump operating point:**

| Parameter | Value |
|-----------|-------|
| Frequency register | 20 (≈ 156 Hz) |
| Voltage register | 180 (≈ 106 Vpp) |

### Control Board (`Control` class)

NTC thermistor (B57164K103J, 10 kΩ @ 25 °C, β = 4300 K) read on GPIO 36. Fan PWM on GPIO 12.

| PCB temperature | Fan speed |
|----------------|-----------|
| > 45 °C | 100 % (255) |
| > 37.5 °C | 75 % (192) |
| > 30 °C | 50 % (128) |
| ≤ 30 °C | 25 % (64) |

---

## Non-blocking Pump Update State Machine

The mp-Lowdriver requires a `STOP → 5 ms settle → write registers → RESUME` sequence whenever voltage or frequency changes. This is handled internally by a per-pump `PumpUpdate` struct driven by `process_Pending_Updates()` every loop. Callers queue changes; the state machine applies them without blocking the control loop.

---

## Build & Upload

### Prerequisites

- VS Code + PlatformIO IDE extension
- ESP32-DEVKIT-V1 board connected via USB

### Commands

```bash
# Build
platformio run

# Build and upload
platformio run --target upload

# Serial monitor (115200 baud)
platformio device monitor --baud 115200
```

---

## Hardware Validation Tests

Each test replaces `src/main.cpp` temporarily. Upload, then observe the serial monitor at 115200 baud.

| Test file | Purpose |
|-----------|---------|
| `test_i2c_scanner.cpp` | Scan both MUX buses; verify all device addresses |
| `test_control_ntc_fans.cpp` | NTC temperature reading + fan PWM at 25/50/75/100 % |
| `test_led_array.cpp` | Cycle each UV LED channel independently |
| `test_environmental_sensors.cpp` | Read SHT35, LTR390, T6615 and print to serial |
| `test_pumps_logic.cpp` | Drive all 4 pumps at varying frequencies |
| `test_ito_glass.cpp` | Step ITO heater PWM from 0 → max |
| `test_wifi_communication.cpp` | Start AP and echo WebSocket messages |
| `test_main.cpp` | Full system integration test |

---

## Troubleshooting

| Symptom | Check |
|---------|-------|
| Sensors read 0 / NaN | Run `test_i2c_scanner.cpp`; verify MUX addresses 0x70 and 0x71 appear |
| Pumps unresponsive | Verify mp-Lowdriver at 0x59 on MUX 0x71 channels 2–5 |
| Temperature oscillates | Verify `ITO_PWM_MAX = 30` is not too restrictive for your ambient |
| UI cannot connect | Confirm `ThermoNoOC` AP is visible; IP must be 192.168.4.1, port 5000 |
| Flow reads zero | Check SLF3S-0600F continuous-measurement start command sent during `begin()` |
