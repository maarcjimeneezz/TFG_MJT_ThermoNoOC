# ThermoNoOC UI - User Interface Documentation

## Overview
Professional control interface for the ThermoNoOC incubator system with real-time monitoring and WiFi communication with ESP32.

## Features

### 1. **Incubator Monitoring** 🌡️
- **Real-time Graphs**: Three interactive matplotlib graphs displaying:
  - Combined Temperature from both Sensor 1 and Sensor 2
  - Combined Humidity from both Sensor 1 and Sensor 2
  - UV Radiation levels
  - CO₂ concentration
- **Live Value Display**: Current readings for all sensors updated every second
- **Color-coded Indicators**: Solid lines for Sensor 1, dashed lines for Sensor 2
- **Data Export**: Button to save all sensor data to CSV file with timestamp

### 2. **Temperature Control** 🌡️
- **Slider Control**: Smooth slider from 20°C to 50°C for setting target temperature
- **Manual Entry**: Type exact temperature values for precise control
- **Real-time Feedback**: Current target temperature displayed during adjustment
- **ESP32 Integration**: Commands sent immediately upon change

### 3. **UV LED Control** 💡
- **4 Independent Groups**: Control up to 4 UV LED arrays independently
- **Toggle Switch**: Enable/Disable each group
- **Intensity Slider**: 0-100% intensity control for each group
- **Individual Status**: Real-time display of each group's intensity level

### 4. **Microfluidics Control** 💧
- **2 Micropumps**: Independent control of two peristaltic pumps
- **Flow Input**: Set desired flow rate (0-100 µL/min) for each pump
- **Flow Monitoring**: Real-time display of actual flow sensor readings
- **Real-time Plotting**: Graph showing flow rate trends over time

### 5. **Data Export** 💾
- **CSV Export**: Save all sensor data to CSV file with automatic timestamp
- **Complete Data**: Exports temperature, humidity, UV, CO₂, and flow rate data
- **One-Click Save**: Simple button to save data anytime during operation

### 6. **ESP32 Connectivity** 🌐
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

### Connecting to ESP32
1. Click the "Connect ESP32" button
2. Ensure ESP32 is powered and in WiFi AP mode
3. Status indicator will turn green when connected
4. Button will change to "Disconnect"

### Setting Temperature
**Option 1 - Slider:**
- Drag the slider to desired temperature (20-50°C)
- Value updates in real-time

**Option 2 - Manual Entry:**
- Type temperature in the input field (must be 20-50°C)
- Click "Set" button

### Controlling UV LEDs
1. Select the UV group (1-4)
2. Use the toggle switch to enable/disable
3. Adjust the intensity slider (0-100%)
4. Changes apply immediately

### Managing Microfluidics
1. Select the pump (1 or 2)
2. Enter desired flow rate (0-100 µL/min)
3. Click "Set Flow"
4. Monitor actual flow in the graph and current flow display

### Exporting Data
1. Click the "💾 Save Data to CSV" button in the Incubator Monitoring section
2. A CSV file will be created with automatic folder structure:
   - **Folder**: `Incubator_Data/YYYYMMDD/`
   - **File**: `incubator_data_YYYYMMDD_HHMMSS.csv`
   - Example: `Incubator_Data/20260506/incubator_data_20260506_143022.csv`
3. The file includes all sensor readings with timestamps
4. Can be imported into Excel, Python, or other analysis tools

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
  "command": "set_flow",
  "pump": 1,
  "flow": 25.5
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
| Temp1 (°C) | °C | Temperature from Sensor 1 |
| Humidity1 (%) | % | Humidity from Sensor 1 |
| Temp2 (°C) | °C | Temperature from Sensor 2 |
| Humidity2 (%) | % | Humidity from Sensor 2 |
| UV (W/m²) | W/m² | UV radiation intensity |
| CO2 (ppm) | ppm | CO₂ concentration level |
| Flow1 (µL/min) | µL/min | Pump 1 flow rate |
| Flow2 (µL/min) | µL/min | Pump 2 flow rate |

Example CSV content:
```
Timestamp,Temp1 (°C),Humidity1 (%),Temp2 (°C),Humidity2 (%),UV (W/m²),CO2 (ppm),Flow1 (µL/min),Flow2 (µL/min)
2026-05-06 14:30:22,37.12,65.34,36.87,70.12,105.23,415.45,25.67,30.34
2026-05-06 14:30:23,37.15,65.28,36.89,70.08,104.98,414.78,25.71,30.31
```

```python
class SensorData:
    - timestamps: Recent timestamps (up to 500 points)
    - temp1/temp2: Temperature readings
    - humidity1/humidity2: Humidity readings
    - uv: UV radiation intensity
    - co2: CO₂ concentration
    - flow1/flow2: Pump flow rates
```

The application maintains a rolling buffer of the last 500 data points for graph display.

## UI Layout

```
┌─────────────────────────────────────────────────────────┐
│  ThermoNoOC Incubator Control System  ● Connected       │
│  [Connect ESP32]                                         │
├─────────────────────────────────────────────────────────┤
│  🌡️ INCUBATOR MONITORING          [💾 Save Data to CSV]│
│  ┌─────────────────────┬─────────────────────────────┐  │
│  │ Temp (S1+S2)        │ UV & CO₂                    │  │
│  │ [Graph]             │ [Graph]                     │  │
│  ├─────────────────────┴─────────────────────────────┤  │
│  │ Humidity (S1+S2)    [Graph - Single Plot]         │  │
│  │ [Sensor Values: Temp, Humidity, UV, CO₂]         │  │
├─────────────────────────────────────────────────────────┤
│  🌡️ TEMPERATURE CONTROL                                 │
│  │ Target Temperature: [========●========] 37.0°C   │  │
│  │ Or enter value: [_________] [Set] (20-50°C)     │  │
├─────────────────────────────────────────────────────────┤
│  💡 UV LED CONTROL (4 Groups)                          │
│  │ Group1   │ Group2   │ Group3   │ Group4          │  │
│  │ [Toggle] │ [Toggle] │ [Toggle] │ [Toggle]        │  │
│  │ [Slider] │ [Slider] │ [Slider] │ [Slider]        │  │
│  │ 50%      │ 50%      │ 50%      │ 50%             │  │
├─────────────────────────────────────────────────────────┤
│  💧 MICROFLUIDICS SECTION                               │
│  │ [Flow Rate Graph - Both Pumps]                    │  │
│  │ Pump 1               │ Pump 2                      │  │
│  │ Flow: [_____] µL/min │ Flow: [_____] µL/min      │  │
│  │ Current: 0.0 µL/min  │ Current: 0.0 µL/min      │  │
│  │ [Set Flow]           │ [Set Flow]                  │  │
└─────────────────────────────────────────────────────────┘
```

## Customization

### Change ESP32 IP Address
Edit in the `IncubatorUI.__init__()` method:
```python
self.esp32_host = "192.168.4.1"  # Change this
self.esp32_port = 80              # Or this
```

### Adjust Temperature Range
Edit in the `create_temperature_control_section()` method:
```python
self.temp_slider = ctk.CTkSlider(
    control_frame,
    from_=20,      # Minimum (currently 20°C)
    to=50,         # Maximum (currently 50°C)
    ...
)
```

### Modify Graph Colors
Edit the color values in:
- `update_temperature_graph()`
- `update_env_graph()`
- `update_flow_graph()`

Colors are defined as hex values (e.g., '#ff6b6b')

### Change UI Theme
In the beginning of the file:
```python
ctk.set_appearance_mode("dark")        # "dark", "light", "system"
ctk.set_default_color_theme("blue")    # "blue", "green", "dark-blue"
```

## Troubleshooting

### Connection Issues
- Ensure ESP32 is powered and in AP mode
- Check IP address matches ESP32 settings
- Verify both devices are on same network
- Check firewall settings

### Graphs Not Updating
- Verify simulation thread is running
- Check matplotlib installation
- Look for error messages in terminal

### Slow Performance
- Reduce graph update frequency (change `self.after(1000)`)
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

- [ ] Data logging to CSV/Database
- [ ] Historical data analysis
- [ ] Automated schedules/recipes
- [ ] Multi-device support
- [ ] Mobile app integration
- [ ] Advanced alerting system
- [ ] User authentication
- [ ] Export/Import configurations
