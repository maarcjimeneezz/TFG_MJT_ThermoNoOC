# ThermoNoOC UI - User Interface Documentation

## Overview
Professional control interface for the ThermoNoOC incubator system with real-time monitoring and WiFi communication with ESP32. Features a modern tabbed single-window design for efficient workspace management.

## Features

### 1. **Tabbed Interface** 📑
- **Single Window Design**: All controls and monitoring in one organized window
- **Navigation Tabs**: Easy switching between different system aspects:
  - **Clock**: System time and status overview
  - **Temperature**: Temperature and humidity monitoring and control
  - **UV Control**: UV LED array management
  - **Microfluidics**: Pump control and flow monitoring

### 2. **System Gating** 🔒
- **Incubator Closed Button**: Activates incubator sensors (temperature, humidity, UV, CO₂)
- **Microfluidics Closed Button**: Activates microfluidics sensors (flow sensors)
- **Independent Control**: Can operate incubator and microfluidics separately
- **Sensor Protection**: No readings displayed when systems are not closed

### 3. **Temperature Control** 🌡️
- **Real-time Monitoring**: Live temperature and humidity from Glass and Base sensors
- **Dynamic Plot Scaling**: Y-axis automatically adjusts to ±4°C around current temperature
- **Target Control**: Slider and manual entry for precise temperature setting (20-50°C)
- **Dual Sensor Display**: Separate readings from glass and base sensors

### 4. **Environmental Monitoring** 🌿
- **CO₂ Levels**: Displayed as percentage instead of ppm
- **UV Radiation**: Real-time UV intensity monitoring
- **Humidity Tracking**: Separate humidity readings from both sensors
- **Real-time Graphs**: Interactive matplotlib plots with color-coded data

### 5. **UV LED Control** 💡
- **4 Independent Groups**: Control up to 4 UV LED arrays independently
- **Toggle Switch**: Enable/Disable each group
- **Intensity Slider**: 0-100% intensity control for each group
- **Individual Status**: Real-time display of each group's intensity level

### 6. **Microfluidics Control** 💧
- **2 Independent Micropumps**: Separate control for each pump (0-2000 µL/min)
- **Per-Pump Modes**:
  - **Continuous**: Steady flow operation
  - **Intermittent**: Timed feeding cycles with configurable parameters
- **Timing Controls** (Intermittent mode only):
  - Feeding time (seconds)
  - Pause time (seconds)
  - Number of cycles
- **Flow Monitoring**: Real-time display of actual flow sensor readings
- **Real-time Plotting**: Graph showing flow rate trends over time

### 7. **Data Export** 💾
- **CSV Export**: Save all sensor data to CSV file with automatic timestamp
- **Complete Data**: Exports temperature, humidity, UV, CO₂, and flow rate data
- **One-Click Save**: Simple button to save data anytime during operation

### 8. **ESP32 Connectivity** 🌐
- **WiFi Connection**: Direct TCP connection to ESP32 (default: 192.168.4.1:80)
- **Connection Status**: Visual indicator (green = connected, red = disconnected)
- **Command Protocol**: JSON-based command system for reliable communication

## Installation

### Prerequisites
- Python 3.7+
- pip package manager

### Setup
```bash
# Navigate to UI directory
cd TFG_MJT_ThermoNoOC/UI

# Install dependencies
pip install -r requirements.txt
```

## Usage

### Running the Application
```bash
python User_interface.py
```

### Initial Setup
1. **System Gating**: Click "Incubator Closed" and/or "Microfluidics Closed" to activate sensors
2. **ESP32 Connection**: Click "Connect ESP32" button when ready
3. **Navigation**: Use tabs to switch between different control sections

### Temperature Control
**Option 1 - Slider:**
- Drag the slider to desired temperature (20-50°C)
- Value updates in real-time

**Option 2 - Manual Entry:**
- Type temperature in the input field (must be 20-50°C)
- Click "Set" button

### UV LED Management
1. Select the UV group (1-4)
2. Use the toggle switch to enable/disable
3. Adjust the intensity slider (0-100%)
4. Changes apply immediately

### Microfluidics Operation
1. **Activate System**: Ensure "Microfluidics Closed" is pressed
2. **Per-Pump Setup**:
   - Choose Continuous or Intermittent mode
   - For Continuous: Set flow rate (0-2000 µL/min)
   - For Intermittent: Configure feeding time, pause time, and cycles
3. **Apply Settings**: Click "Set Flow & Mode" for each pump
4. **Monitor**: Watch real-time flow in graphs and current flow display

### Data Export
1. Click the "💾 Save Data to CSV" button in the Temperature tab
2. A CSV file will be created with automatic folder structure:
   - **Folder**: `Incubator_Data/YYYYMMDD/`
   - **File**: `incubator_data_YYYYMMDD_HHMMSS.csv`
   - Example: `Incubator_Data/20260506/incubator_data_20260506_143022.csv`

## Command Protocol (JSON)

### Temperature Control
```json
{
  "command": "set_temp",
  "value": 37.5
}
```

### UV Control
```json
{
  "command": "uv_toggle",
  "group": 1,
  "enabled": true
}
```

```json
{
  "command": "uv_intensity",
  "group": 1,
  "intensity": 75
}
```

### Microfluidics Control
```json
{
  "command": "set_pump_flow_and_mode",
  "pump": 1,
  "flow": 500,
  "mode": "continuous"
}
```

```json
{
  "command": "set_pump_flow_and_mode",
  "pump": 1,
  "flow": 1000,
  "mode": "intermittent",
  "feeding_time": 30,
  "pause_time": 10,
  "cycles": 5
}
```

## Data Storage & Export

### Directory Structure
All data files are organized in a hierarchical folder structure:
```
Incubator_Data/
├── 20260506/           # Date-based subfolder (YYYYMMDD)
│   ├── incubator_data_20260506_143022.csv
│   ├── incubator_data_20260506_145530.csv
│   └── incubator_data_20260506_150145.csv
├── 20260507/
│   ├── incubator_data_20260507_090000.csv
│   └── ...
└── ...
```

### CSV Export Format

When you save data to CSV, the file includes the following columns:

| Column | Unit | Description |
|--------|------|-------------|
| Timestamp | YYYY-MM-DD HH:MM:SS | Exact time of data reading |
| Glass Sensor Temp (°C) | °C | Temperature from glass sensor |
| Base Sensor Temp (°C) | °C | Temperature from base sensor |
| Glass Sensor Humidity (%) | % | Humidity from glass sensor |
| Base Sensor Humidity (%) | % | Humidity from base sensor |
| UV (W/m²) | W/m² | UV radiation intensity |
| CO₂ (%) | % | CO₂ concentration level |
| Pump 1 Flow (µL/min) | µL/min | Pump 1 flow rate |
| Pump 2 Flow (µL/min) | µL/min | Pump 2 flow rate |

Example CSV content:
```
Timestamp,Glass Sensor Temp (°C),Base Sensor Temp (°C),Glass Sensor Humidity (%),Base Sensor Humidity (%),UV (W/m²),CO₂ (%),Pump 1 Flow (µL/min),Pump 2 Flow (µL/min)
2026-05-06 14:30:22,37.12,36.87,65.34,70.12,105.23,0.042,25.67,30.34
2026-05-06 14:30:23,37.15,36.89,65.28,70.08,104.98,0.041,25.71,30.31
```

## Configuration

The application uses `config.py` for all customizable settings:

### Key Configuration Sections
- **ESP32_CONFIG**: Connection parameters
- **TEMPERATURE_CONFIG**: Temperature control settings and plot scaling
- **MICROFLUIDICS_CONFIG**: Pump settings and timing defaults
- **SENSOR_CONFIG**: Sensor labels and display units
- **GATING_CONFIG**: Default gating states and sensor groupings
- **SIMULATION_CONFIG**: Test data generation parameters

### Customization Examples
```python
# Change temperature plot range
TEMPERATURE_CONFIG["plot_range_degrees"] = 8

# Modify sensor labels
SENSOR_CONFIG["temp1"]["label"] = "Glass Sensor"

# Adjust microfluidics range
MICROFLUIDICS_CONFIG["max_flow"] = 2000
```

## UI Layout

```
┌─────────────────────────────────────────────────────────┐
│  ThermoNoOC Incubator Control System  ● Connected       │
│  [Incubator Closed] [Microfluidics Closed]              │
├─────────────────────────────────────────────────────────┤
│  [Clock] [Temperature] [UV Control] [Microfluidics]     │
│                                                         │
│  🌡️ TEMPERATURE TAB                                    │
│  ┌─────────────────────┬─────────────────────────────┐  │
│  │ Temp Plot (8° range) │ Humidity & UV & CO₂ Plot   │  │
│  │ [Graph]             │ [Graph]                     │  │
│  ├─────────────────────┴─────────────────────────────┤  │
│  │ Target Temperature: [========●========] 37.0°C   │  │
│  │ Or enter value: [_________] [Set] (20-50°C)     │  │
│  │ [💾 Save Data to CSV]                             │  │
├─────────────────────────────────────────────────────────┤
│  💧 MICROFLUIDICS TAB                                  │
│  │ [Flow Rate Graph]                                  │  │
│  │ Pump 1                    │ Pump 2                 │  │
│  │ [Continuous] [Intermittent] │ [Continuous] [Interm]│  │
│  │ Flow: [_____] µL/min      │ Flow: [_____] µL/min │  │
│  │ Feeding: [__] Pause: [__] │ Feeding: [__] Pause:[__]│  │
│  │ Cycles: [__]              │ Cycles: [__]          │  │
│  │ [Set Flow & Mode]         │ [Set Flow & Mode]      │  │
└─────────────────────────────────────────────────────────┘
```

## Troubleshooting

### Connection Issues
- Ensure ESP32 is powered and in WiFi AP mode
- Check IP address matches ESP32 settings
- Verify both devices are on same network
- Check firewall settings

### No Sensor Readings
- Verify "Incubator Closed" and/or "Microfluidics Closed" buttons are pressed
- Check ESP32 connection status
- Ensure simulation is enabled in config.py for testing

### Graphs Not Updating
- Verify simulation thread is running
- Check matplotlib installation
- Look for error messages in terminal

### Microfluidics Timing Fields Disabled
- Timing fields (feeding time, pause time, cycles) are only enabled in "Intermittent" mode
- Switch to intermittent mode to configure timing parameters

### Slow Performance
- Reduce graph update frequency (change `GRAPH_CONFIG["update_interval"]`)
- Limit the number of data points stored
- Close other memory-intensive applications

## Integration with ESP32

The UI communicates with ESP32 via:
- **Protocol**: TCP JSON commands
- **Default IP**: 192.168.4.1
- **Default Port**: 80
- **Format**: UTF-8 encoded JSON strings

Implement corresponding endpoints in your ESP32 firmware to handle the command JSON packets.

## Future Enhancements

- [ ] Data logging to database
- [ ] Historical data analysis
- [ ] Automated schedules/recipes
- [ ] Multi-device support
- [ ] Mobile app integration
- [ ] Advanced alerting system
- [ ] User authentication
- [ ] Export/Import configurations