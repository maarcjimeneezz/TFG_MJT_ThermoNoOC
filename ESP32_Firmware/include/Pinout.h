#ifndef PINOUT_H
#define PINOUT_H

#include <Arduino.h>

// ==========================================
// 1. MAIN I2C BUS & MULTIPLEXERS
#define I2C_SDA 21
#define I2C_SCL 22

// Multiplexer Addresses
#define MUX_ADDR_SENSORS 0x70       // A0,A1,A2 -> GND
#define MUX_ADDR_MICROFLUIDICS 0x71 // A0 -> 5V, A1,A2 -> GND

// ==========================================
// 2. CONTROL BOARD (Direct ESP32 Pins)
#define PIN_TEMP_PCB 36    // VP (Analog Input for NTC)
#define PIN_ITO_CONTROL 32 // PWM for ITO Glass
#define PIN_PWM_FANS 12    // PWM for Fans

// LED Array PWM Channels (Note: D18, D19, D25, D26)
#define PIN_C1_LEDS 18
#define PIN_C2_LEDS 19
#define PIN_C3_LEDS 25
#define PIN_C4_LEDS 26

// ==========================================
// 3. SENSORS BOARD (via MUX 0x70 + UART)
// I2C Channels on Mux 0x70
#define MUX_CH_TEMP1 2
#define MUX_CH_TEMP2 3
#define MUX_CH_UV 4

// CO2 Sensor (Dedicated UART2)
#define PIN_CO2_RX 16 // RX2
#define PIN_CO2_TX 17 // TX2

// ==========================================
// 4. MICROFLUIDICS BOARD (via MUX 0x71)
// I2C Channels on Mux 0x71
#define MUX_CH_PUMP_1 2
#define MUX_CH_PUMP_2 3
#define MUX_CH_PUMP_3 4
#define MUX_CH_PUMP_4 5
#define MUX_CH_FLOW_1 6
#define MUX_CH_FLOW_2 7

#endif