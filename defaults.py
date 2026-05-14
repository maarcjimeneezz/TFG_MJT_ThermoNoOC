"""
defaults.py  –  ThermoNoOC shared default values
These MUST stay in sync with the initialisers in Incubator.h / main.cpp
"""

# Connection parameters for the ESP32 microcontroller
ESP32_IP = "192.168.0.132"
ESP32_SSID = "ThermoNoOC"
ESP32_PASSWORD = "thermonooc"
ESP32_PORT = 5000
ESP32_TIMEOUT = 2.0  # seconds

# ITO Glass Temperature
DEFAULT_TARGET_TEMP = 20.0          # °C   (Temperature selected in order to have the incubator off at the start of the experiment)

# UV LED groups (indices 0-3)
DEFAULT_UV_ENABLED   = [False, False, False, False]   # all off
DEFAULT_UV_INTENSITY = [0, 0, 0, 0]                   # 0-100 percent

# Pump circuits (indices 0-1)
DEFAULT_PUMP_FLOW         = [0.0,          0.0]           # µL/min
DEFAULT_PUMP_MODE         = ["continuous", "continuous"]
DEFAULT_PUMP_FEEDING_TIME = [0.0,          0.0]           # seconds
DEFAULT_PUMP_PAUSE_TIME   = [0.0,          0.0]           # seconds
DEFAULT_PUMP_CYCLES       = [0,            0]