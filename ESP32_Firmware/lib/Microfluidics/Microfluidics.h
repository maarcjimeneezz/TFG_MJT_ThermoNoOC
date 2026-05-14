#ifndef MICROFLUIDICS_H
#define MICROFLUIDICS_H

#include <Arduino.h>
#include <Wire.h>
#include "Pinout.h"

/**
 * @class Microfluidics
 * @brief Controls 4 Bartels mp-Lowdriver peristaltic pumps and 2 Sensirion SLF3S-0600F
 *        flow sensors via I2C multiplexer MUX_ADDR_MICROFLUIDICS (0x71).
 *
 * Single responsibility: pump configuration, continuous/pulsed flow logic, and
 * flow sensor reading. No incubator sensor data, LED, fan, or WiFi logic.
 *
 * Circuit organisation:
 *   Circuit 1 — pump 1 (fluid) + pump 2 (air bubble removal)
 *   Circuit 2 — pump 3 (fluid) + pump 4 (air bubble removal)
 *
 * Non-blocking pump updates:
 *   The mp-Lowdriver requires a STOP → [5 ms settle] → write → RESUME sequence
 *   when changing voltage or frequency. This is handled internally via a per-pump
 *   pending-update state machine driven by update_Pumps(). Callers never block.
 *
 * mp-Lowdriver specs: 0-150 Vpp, 8-2000 Hz
 * SLF3S-0600F specs:  0-2000 µL/min
 */
class Microfluidics
{
public:
    /** Per-circuit pump configuration. Written by command parser in main.cpp. */
    struct PumpConfig
    {
        float flowRate_uLmin = 0.0f; // Desired flow rate (0-2000 µL/min); 0 = off
        bool pulsed = false;         // false = continuous, true = pulsed
        float feedTime_s = 0.0f;     // Feed duration per cycle  (pulsed mode)
        float pauseTime_s = 0.0f;    // Pause duration per cycle (pulsed mode)
        int cycles = 0;              // Total cycles to run; 0 = infinite
    };

private:
    static const uint8_t PUMP_I2C_ADDR = 0x59; // mp-Lowdriver default address
    static const uint8_t FLOW_I2C_ADDR = 0x08; // SLF3S-0600F default address
    static const int NUM_CIRCUITS = 2;
    static const int NUM_PUMPS = 4;
    static const int NUM_SENSORS = 2;

    static const uint8_t PUMP_MUX_CH[NUM_PUMPS]; // Defined in .cpp
    static const uint8_t FLOW_MUX_CH[NUM_SENSORS];

    // Flow rate → pump frequency mapping (0-2000 µL/min → 8-800 Hz)
    static constexpr float FLOW_TO_HZ = 2.5f;
    static const uint16_t FREQ_MIN_HZ = 8;
    static const uint16_t FREQ_MAX_HZ = 800;
    // mp-Lowdriver frequency register: 1 LSB = 7.8125 Hz
    static constexpr float HZ_PER_BIT = 7.8125f;
    // Required settle time between STOP and parameter write (mp-Lowdriver datasheet)
    static const unsigned long PUMP_SETTLE_MS = 5;

    // ---- Per-circuit state for pulsed mode ----
    PumpConfig _config[NUM_CIRCUITS];
    unsigned long _lastCycleTime[NUM_CIRCUITS];
    int _cycleCount[NUM_CIRCUITS];

    // ---- Non-blocking pump update state machine (one entry per physical pump) ----
    struct PumpUpdate
    {
        bool active = false;
        uint8_t voltage = 0;  // Pending amplitude byte
        uint8_t freqByte = 0; // Pending frequency register byte
        bool hasVoltage = false;
        bool hasFreq = false;
        unsigned long stopSentAt = 0;
    };
    PumpUpdate _pending[NUM_PUMPS];
    uint8_t _curVoltage[NUM_PUMPS]; // Last-applied amplitude (avoids redundant writes)
    uint8_t _curFreqByte[NUM_PUMPS];

    // ---- Low-level hardware helpers ----
    void select_Mux_Bus(uint8_t muxChannel);
    void set_Pump_Register_Page(uint8_t page);
    void send_Pump_Stop(int pumpIdx);
    void send_Pump_Resume(int pumpIdx);
    void apply_Pending_Update(int pumpIdx);
    void init_Single_Pump(int pumpIdx);
    void init_Single_Flow_Sensor(int sensorIdx);

    // ---- Pending-update queue helpers ----
    void queue_Pump_Voltage(int pumpIdx, uint8_t voltage);
    void queue_Pump_Freq_Byte(int pumpIdx, uint8_t freqByte);
    void process_Pending_Updates();

    // ---- Circuit logic helpers ----
    uint8_t flow_Rate_To_Freq_Byte(float flowRate_uLmin) const;
    void stop_Circuit_Pumps(int circuitIdx);
    void run_Circuit_Pumps(int circuitIdx);
    void apply_Circuit_Continuous(int circuitIdx);
    void apply_Circuit_Pulsed(int circuitIdx);

public:
    Microfluidics();

    /** Initialises all 4 pumps and 2 flow sensors. Call once in setup(). */
    void begin();

    /**
     * Sets the configuration for a fluidic circuit (1 or 2).
     * Resets pulsed-mode counters when called.
     */
    void set_Circuit_Config(int circuit, const PumpConfig &config);

    /**
     * Non-blocking update: processes pending pump parameter changes (stop→settle→write→resume)
     * and applies circuit continuous/pulsed logic. Call every loop().
     */
    void update_Pumps();

    /** Reads flow rate from sensor 1 or 2. Returns µL/min. */
    float read_Flow_Rate(int sensorNum);

    // Direct low-level pump control (bypasses circuit logic)
    void set_Pump_Voltage(int pumpNum, uint8_t voltage);
    void set_Pump_Frequency(int pumpNum, uint16_t frequency_Hz);
};

#endif // MICROFLUIDICS_H
