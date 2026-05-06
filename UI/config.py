"""
Configuration file for ThermoNoOC Incubator UI
Modify these settings to customize the application
"""

# ESP32 Connection Settings
ESP32_CONFIG = {
    "host": "192.168.4.1",      # ESP32 AP IP address
    "port": 80,                  # ESP32 server port
    "timeout": 2,                # Connection timeout (seconds)
}

# Temperature Control Settings
TEMPERATURE_CONFIG = {
    "min": 20,                   # Minimum temperature (°C)
    "max": 50,                   # Maximum temperature (°C)
    "default": 37,               # Default target temperature (°C)
    "steps": 150,                # Number of slider steps
}

# UV LED Configuration
UV_CONFIG = {
    "num_groups": 4,             # Number of UV LED groups
    "min_intensity": 0,          # Minimum intensity (%)
    "max_intensity": 100,        # Maximum intensity (%)
    "default_intensity": 50,     # Default intensity (%)
    "intensity_steps": 100,      # Number of slider steps
}

# Microfluidics Configuration
MICROFLUIDICS_CONFIG = {
    "num_pumps": 2,              # Number of micropumps
    "min_flow": 0,               # Minimum flow (µL/min)
    "max_flow": 100,             # Maximum flow (µL/min)
    "default_flow": 0,           # Default flow (µL/min)
}

# Graph Configuration
GRAPH_CONFIG = {
    "max_data_points": 500,      # Maximum points to display in graphs
    "update_interval": 1000,     # Graph update interval (milliseconds)
    "dpi": 100,                  # Graph DPI for quality
    "figure_size": {
        "temp": (7, 3.5),        # Temperature graph size
        "env": (7, 3.5),         # Environment graph size
        "flow": (14, 3),         # Flow graph size
    }
}

# UI Theme Configuration
THEME_CONFIG = {
    "appearance_mode": "dark",   # "dark", "light", "system"
    "color_theme": "blue",       # "blue", "green", "dark-blue"
    "font_family": "Helvetica",  # Default font
}

# Graph Colors
COLOR_CONFIG = {
    "temp1": "#ff6b6b",         # Red
    "humidity1": "#4ecdc4",     # Cyan
    "temp2": "#ff6b6b",         # Red
    "humidity2": "#4ecdc4",     # Cyan
    "uv": "#ffd93d",            # Yellow
    "co2": "#95e1d3",           # Mint
    "flow1": "#6c5ce7",         # Purple
    "flow2": "#a29bfe",         # Light Purple
    "success": "#44ff44",       # Green
    "error": "#ff4444",         # Red
    "warning": "#ffaa00",       # Orange
}

# Sensor Display Configuration
SENSOR_CONFIG = {
    "temp1": {"label": "Temp 1", "unit": "°C", "decimal_places": 1},
    "humidity1": {"label": "Humidity 1", "unit": "%", "decimal_places": 1},
    "temp2": {"label": "Temp 2", "unit": "°C", "decimal_places": 1},
    "humidity2": {"label": "Humidity 2", "unit": "%", "decimal_places": 1},
    "uv": {"label": "UV", "unit": "W/m²", "decimal_places": 1},
    "co2": {"label": "CO₂", "unit": "ppm", "decimal_places": 0},
}

# Data Simulation Settings (for testing without ESP32)
SIMULATION_CONFIG = {
    "enabled": True,             # Enable data simulation
    "temp1_center": 37,         # Center temperature sensor 1
    "temp1_variance": 0.5,      # Variance
    "temp2_center": 36.8,       # Center temperature sensor 2
    "temp2_variance": 0.5,      # Variance
    "humidity1_center": 65,     # Center humidity sensor 1
    "humidity1_variance": 5,    # Variance
    "humidity2_center": 70,     # Center humidity sensor 2
    "humidity2_variance": 5,    # Variance
    "uv_center": 100,           # Center UV level
    "uv_variance": 20,          # Variance
    "co2_center": 400,          # Center CO2 level
    "co2_variance": 50,         # Variance
    "flow1_center": 25,         # Center flow pump 1
    "flow1_variance": 2,        # Variance
    "flow2_center": 30,         # Center flow pump 2
    "flow2_variance": 2,        # Variance
}

# Window Configuration
WINDOW_CONFIG = {
    "title": "ThermoNoOC Incubator Control System",
    "width": 1600,
    "height": 1000,
    "min_width": 1400,
    "min_height": 900,
}

# Data Logging Configuration
LOGGING_CONFIG = {
    "enabled": True,             # Enable data logging
    "data_folder": "Incubator_Data",  # Root folder for data storage
    "use_date_subfolder": True,  # Create YYYYMMDD subfolders
    "log_interval": 5,           # Log every N seconds (reserved for future auto-logging)
}
