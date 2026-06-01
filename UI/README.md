# ThermoNoOC — Desktop Control Panel

**File**: `user-interface-v2.py`  
**Requires**: Python 3.9+  
**Dependencies**: `customtkinter`, `matplotlib`, `websocket-client`

---

## Installation

```bash
pip install customtkinter matplotlib websocket-client
python user-interface-v2.py
```

---

## Connection

The UI connects to the ESP32 over WebSocket. The ESP32 must be powered and broadcasting its Access Point before connecting.

| Parameter | Value |
|-----------|-------|
| Network (join first) | `ThermoNoOC` / `thermonooc` |
| WebSocket URL | `ws://192.168.4.1:5000` |

1. Join the **ThermoNoOC** WiFi network on the host PC.
2. Click **Connect** in the header bar.
3. The status dot turns green and shows **Connected**.

---

## Header Bar

| Element | Description |
|---------|-------------|
| Tab buttons | Switch between Incubator / UV Light / Microfluidics |
| Clock | Live HH:MM:SS clock |
| Status dot | Grey = disconnected · Yellow = connecting · Green = connected |
| **Connect / Disconnect** | Toggle WebSocket connection |
| **Send Data** | Transmits current tab's settings to the ESP32 (enabled only when the relevant interlock is closed and a change has been made) |
| ☀ / 🌙 | Toggle Dark / Light theme |

---

## Safety Interlocks

Two independent toggle buttons gate all actuator activity. They are visible and operable from every tab that needs them.

| Button | Variable | Effect when **opened** |
|--------|----------|----------------------|
| **Incubator Opened/Closed** | `is_Incubator_Closed` (ESP32) | ESP32 stops heater, LEDs, fans, and pumps. Send Data disabled. |
| **Microfluidics Opened/Closed** | `is_Micro_Closed` (ESP32) | ESP32 immediately stops all 4 pumps. Send Data disabled on Microfluidics tab. |

Both buttons follow the same visual convention: **red = opened**, **green = closed**.

---

## Tab: Incubator

Controls and monitors the biological incubation environment.

**Interlock**: Incubator Opened/Closed button at the top.

### Temperature section

- Live plot — top and bottom SHT35 sensor readings (°C), 120-sample scrolling window
- Info box — current value for each sensor in large numerals
- **Target Temperature slider** — 20–60 °C; changing it marks a pending change (Send Data turns blue)

### Humidity section

- Live plot — top and bottom SHT35 humidity readings (%)
- Info box — current value for each sensor

### CO₂ section

- Live plot — T6615 CO₂ concentration (%)
- Info box — current value in ppm (value × 10 000)

---

## Tab: UV Light

Controls and monitors the UV irradiance environment.

**Interlock**: shared Incubator interlock (same flag gates LEDs on the ESP32).

### Irradiance plot

- Live LTR390 irradiance (W/m²), 120-sample scrolling window

### UV LED Groups (2 × 2 grid)

Each of the 4 groups has:
- **Enable switch** — activates the group; disabling also disables its slider
- **Intensity slider** — 0–100 %; changing any value marks a pending change

### Info box

- **UVI Index** — raw LTR390 UV index value
- **Irradiance** — W/m² with 4-decimal precision

---

## Tab: Microfluidics

Controls and monitors the two fluidic circuits.

**Interlocks**: Both the Incubator and the Microfluidics buttons appear side by side. The Microfluidics button is independent; closing it enables the pumps and activates Send Data for this tab.

### Flow sensor section

- Live plot — Flow Circuit 1 (teal) and Flow Circuit 2 (purple), µL/min
- Info box — current µL/min reading for each circuit

### Pump configuration (2 cards)

Each card represents one fluidic circuit (Pump 1 + 2, and Pump 3 + 4 respectively). The segmented button selects the feeding mode.

#### Continuous mode

| Field | Range | Description |
|-------|-------|-------------|
| Flow Rate | 0–2000 µL/min | Target flow; PID on the fluid pump tracks this setpoint |

#### Pulsed mode

| Field | Unit | Description |
|-------|------|-------------|
| Volume | µL | Volume delivered per pulse |
| Feeding time | s | Duration pump is on per cycle |
| Non-feeding time | s | Duration pump is off per cycle |

All entry fields are **numeric only** (digits and at most one decimal point). Non-numeric input is silently stripped.

Switching mode or editing any value marks a pending change; Send Data becomes active once the Microfluidics interlock is closed.

---

## Send Data

The **Send Data** button transmits the current tab's configuration to the ESP32 via WebSocket. Its state follows these rules:

| Condition | Button state |
|-----------|-------------|
| Relevant interlock is **opened** | Disabled (grey) |
| Interlock **closed**, no changes | Enabled, idle colour |
| Interlock **closed**, pending changes | Enabled, blue (ready) |
| After successful send | Enabled, idle colour |

Switching tabs immediately re-evaluates the button state for the new tab.

### Commands sent

| Tab | Command(s) sent |
|-----|----------------|
| Incubator | `SET_TEMP:<float>` (if changed) |
| UV Light | `SET_LED:<group>:<en>:<intensity>` for each changed group |
| Microfluidics | `SET_PUMP:<circuit>:<flow>:<pulsed>:<feed_s>:<pause_s>:0` for each changed circuit |

Deduplication is applied — only values that differ from the last confirmed send are transmitted.

---

## Data Logging

CSV files are written automatically to `UI/Data_Logging/YYYYMMDD/YYYYMMDD_HHMMSS.csv` whenever the WebSocket is connected **and** the Incubator interlock is closed. Each row is written when a telemetry packet is received (≈ 1 Hz).

### CSV columns

```
timestamp, temp1_C, temp2_C, hum1_pct, hum2_pct,
co2_pct, uvW_Wm2, uvIndex, target_temp_C,
led1_en, led1_int_pct, led2_en, led2_int_pct,
led3_en, led3_int_pct, led4_en, led4_int_pct
```

A new file is created each time a connection is established.

---

## Application Architecture

The UI is a single Python file built with **CustomTkinter** for the window/widget layer and **Matplotlib** (TkAgg backend) for live charts. WebSocket communication runs in a **daemon thread** (`WSClient`); all callbacks post back to the main thread via `self.after(0, ...)` to respect Tkinter's single-thread model.

```
App (CTk root)
├── Header bar          — tabs, clock, status, Send Data, theme toggle
└── CTkScrollableFrame  — one sheet per tab, only the active one is grid-mapped
    ├── Incubator sheet     — temp / humidity / CO₂ plots + controls
    ├── UV Light sheet      — irradiance plot + 4 LED group cards
    └── Microfluidics sheet — flow plot + 2 pump configuration cards
```

### Key internal objects

| Attribute | Type | Purpose |
|-----------|------|---------|
| `_ws` | `WSClient` | WebSocket thread wrapper |
| `_bufs` | `dict[str, deque]` | 120-point rolling buffers per telemetry channel |
| `_pending_changes` | `bool` | Any unsent control change exists |
| `_incubator_closed` | `bool` | Local mirror of incubator interlock state |
| `_micro_closed` | `bool` | Local mirror of microfluidics interlock state |
| `_pump_mode_vars` | `list[StringVar]` | Mode selection per pump card |
| `_pump_vars` | `list[dict[str, StringVar]]` | Numeric entry values per pump card |

### Chart refresh

Charts are redrawn on every telemetry packet (via `self.after(0, self._redraw)`) and also on a 1-second fallback timer (`_tick`). Only the active tab's charts are drawn; all others are skipped to reduce CPU load. Charts are blanked while the relevant interlock is open.
