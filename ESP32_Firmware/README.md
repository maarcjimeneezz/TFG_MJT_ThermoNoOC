This **README.md** is designed to provide a professional overview of your TFG firmware repository. It organizes the project's architecture, explains the modular library system, and details the validation suite you've built for the assembly phase.

---

# ESP32 Firmware: Thermo-Microfluidic Control System

This repository contains the modular C++/Arduino firmware for an ESP32-based controller designed for an integrated incubator and microfluidic system. The project is structured using **PlatformIO** to ensure high maintainability and scalability.

## 📂 Project Structure

The source code is organized into a modular hierarchy to separate hardware concerns from high-level logic:

* **`include/`**: Contains global definitions and the central `Pinout.h` file, ensuring all hardware pins are managed in one location.
* **`lib/`**: The core of the project, divided into specialized hardware drivers:
    * `Control`: Manages the NTC temperature sensor and cooling fan PWM.
    * `Incubator`: Interfaces with environmental sensors (SHT35 for temp/humidity, LTR390 for UV, and T6615 for CO2).
    * `LED_Array`: Controls the 4-channel UV LED lighting system.
    * `Microfluidics`: Implements the I2C protocol for the **mp-Lowdriver** to control 4 micropumps and read 2 flow sensors.
* **`src/`**: Contains `main.cpp`, the final application logic that orchestrates all modules.
* **`test/`**: A comprehensive hardware validation suite used during the PCB assembly and soldering phase.

---

## 🛠 Hardware Drivers (mp-Lowdriver)

The microfluidic control is based on the **BP7 micropump series** and the **mp-Lowdriver**. Key technical characteristics implemented in the `Microfluidics` library include:

* **Communication**: I2C protocol using slave address `0x59`.
* **Voltage Control**: Adjustable amplitude from **0 to 150 Vpp** in 255 steps.
* **Frequency Control**: Adjustable from **0 to 800 Hz** (guaranteed range) in steps of **7.8125 Hz**.
* **Equation for Frequency**: $f_{reg} = \frac{frequency}{7.8125}$.

---

## 🧪 Hardware Validation Suite (`test/`)

To ensure every component is correctly soldered and functional, the following tests are available in the `test/` directory. These are intended to be run individually as the assembly progresses.

### 1. I2C Deep Scanner
`test_i2c_scanner.cpp`
Scans both I2C multiplexers (`0x70` and `0x71`) and enters every sub-channel (0-7) to verify that devices like the pumps, flow sensors, and environmental sensors are correctly detected.

### 2. Control & Thermal Test
`test_control_ntc_fans.cpp`
Validates the analog NTC circuit and the PWM driver for the cooling fans. It tests the fans at different speeds to verify MOSFET saturation and power stability.

### 3. LED Array Validation
`test_led_array.cpp`
Independently fades each of the 4 UV LED channels. This is crucial for detecting bridge-solder shorts between channels or faulty MOSFETs.

### 4. Environmental Monitoring
`test_environmental_sensors.cpp`
Polls the SHT35, LTR390, and T6615 sensors. It confirms that the atmospheric monitoring for the incubator is accurate before biological integration.

### 5. Parallel Pump Stress Test
`test_pumps_logic.cpp`
Simultaneously runs all 4 pumps at different frequencies:
* **Pumps 1 & 3**: Optimized for liquid flow (**80 Hz**).
* **Pumps 2 & 4**: Optimized for air/bubble trap displacement (**300 Hz**).
This test verifies the PCB's power delivery under a "worst-case scenario" load.

---

## 🚀 Getting Started

1.  Open the folder in **VS Code** with the **PlatformIO** extension installed.
2.  To run a test: Copy the desired test code from `test/` into `src/main.cpp` (backup your original main first) and click **Upload**.
3.  Monitor the output via the **Serial Monitor** at **115200 baud**.
