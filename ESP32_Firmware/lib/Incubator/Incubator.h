#ifndef INCUBATOR_H
#define INCUBATOR_H

#include <Arduino.h>
#include <Wire.h>
#include <SHTSensor.h>
#include <Adafruit_LTR390.h>
#include "Pinout.h"

/**
 * @class Incubator
 * @brief Manages the biological incubation environment.
 *
 * Single responsibility: dual SHT35 (temp/humidity), LTR390 (UV index),
 * T6615 (CO2 via UART), and ITO glass heater PWM.
 * No pump, LED, fan, or WiFi logic.
 *
 * CO2 reading is non-blocking: a two-state machine sends the command on one
 * loop tick and parses the response on a subsequent tick, with timeout handling.
 */
class Incubator
{
private:
    // ITO glass heater PWM (Channel 5 — channels 0-4 used by LEDs and fans)
    static const int ITO_FREQ_HZ = 5000;
    static const int ITO_RES_BIT = 8;
    static const int ITO_PWM_CH = 5;

    // PID gains — tuned for dual-crystal load (2× thermal mass, same actuator)
    static constexpr float ITO_KP             = 8.0f;
    static constexpr float ITO_KI             = 0.1f;
    static constexpr float ITO_KD             = 2.0f;
    static constexpr float ITO_INTEGRAL_LIMIT = 40.0f;  // anti-windup clamp
    static constexpr float ITO_DERIV_ALPHA    = 0.1f;   // low-pass for noisy sensor

    // Hard PWM output cap — limits duty cycle to 50 % (128/255) for 60 Ω glass.
    static const uint8_t ITO_PWM_MAX = 128;  // 50 %

    // Max setpoint change rate (°C/s) — faster ramp for dual-crystal configuration
    static constexpr float ITO_RAMP_RATE_CS = 2.0f;

    // LTR390 sensitivity factors for Gain=18x, Resolution=20-bit
    static constexpr float LTR390_SENSITIVITY = 2300.0f;
    static constexpr float LTR390_UVI_TO_WM2 = 0.025f;

    // T6615 CO2 non-blocking state machine
    enum class CO2State : uint8_t
    {
        IDLE,
        CMD_SENT
    };
    static constexpr unsigned long CO2_TIMEOUT_MS = 500;

    SHTSensor _sht1;
    SHTSensor _sht2;
    Adafruit_LTR390 _ltr;

    CO2State _co2State;
    unsigned long _co2CmdTime;

    // PID state
    float         _integral;
    float         _prevError;
    float         _filteredDeriv;
    float         _rampedTarget;
    unsigned long _lastHeaterMs;

    void select_Sensor_Bus(uint8_t muxChannel);
    void read_SHT35_Sensors();
    void read_UV_Sensor();
    void tick_CO2_State_Machine();

public:
    // --- Live sensor readings (populated by read_All_Sensors()) ---
    float temp1, hum1;  // SHT35 #1 — temperature (°C) and humidity (%RH)
    float temp2, hum2;  // SHT35 #2
    float uvIndex;      // LTR390 UV Index (0-11+)
    float uvIrradiance; // LTR390 irradiance (W/m²)
    float co2Percent;   // T6615 CO₂ concentration (0-20 %)

    // --- Setpoint (written by command parser in main.cpp) ---
    float targetTemperature;

    Incubator();

    /** Initialises I2C bus, Serial2 for CO2, and ITO PWM. Call once in setup(). */
    void begin();

    /**
     * Non-blocking sensor poll. Reads SHT35 and LTR390 directly; advances the
     * CO2 state machine (send command OR parse response). Call every loop().
     */
    void read_All_Sensors();

    /** Directly sets ITO glass heater duty cycle (0-255). */
    void set_ITO_Power(uint8_t power);

    /** PID controller with setpoint ramp. Drives ITO duty cycle toward targetTemperature. Call every loop(). */
    void update_Heater_PWM();
};

#endif // INCUBATOR_H