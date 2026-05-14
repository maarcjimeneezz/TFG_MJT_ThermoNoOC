#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>
#include "Pinout.h"

/**
 * @class Control
 * @brief Manages PCB thermal health: NTC thermistor reading and cooling fan PWM.
 *
 * Single responsibility: PCB-level hardware only. No knowledge of incubator
 * sensors, pumps, LEDs, or WiFi.
 */
class Control
{
private:
    // Fan PWM (25 kHz is inaudible and standard for fans)
    static const int FAN_FREQ_HZ = 25000;
    static const int FAN_RES_BIT = 8;
    static const int FAN_PWM_CH = 4; // Channels 0-3 reserved for LED_Array

    // NTC B57164K103J datasheet constants
    static constexpr float NTC_R0 = 10000.0f; // Nominal resistance at T0 (Ω)
    static constexpr float NTC_T0 = 298.15f;  // Reference temp (K = 25 °C)
    static constexpr float NTC_BETA = 4300.0f;
    static constexpr float NTC_RFIXED = 4700.0f; // Voltage-divider series resistor (Ω)
    static constexpr float ADC_VREF = 3.3f;
    static const int ADC_MAX = 4095;   // 12-bit full-scale
    static const int ADC_SAMPLES = 15; // Averaged to suppress ADC noise

    // Linear fan curve: fully off below OFF_TEMP, fully on above FULL_TEMP
    static constexpr float FAN_TEMP_OFF = 35.0f;  // °C
    static constexpr float FAN_TEMP_FULL = 60.0f; // °C

    int read_NTC_ADC_Average() const;
    float convert_ADC_To_Temperature(int adcAvg) const;

public:
    Control();

    /** Configures LEDC PWM and ADC attenuation. Call once in setup(). */
    void begin();

    /** Samples NTC thermistor (15-point average) and returns PCB temp in °C. Non-blocking. */
    float read_PCB_Temperature() const;

    /** Directly sets fan duty cycle (0 = off, 255 = full speed). */
    void set_Fan_Speed(uint8_t speed);

    /** Maps pcbTempC to fan duty cycle using a linear curve. Call every loop(). */
    void update_Fan_Speed(float pcbTempC);
};

#endif // CONTROL_H