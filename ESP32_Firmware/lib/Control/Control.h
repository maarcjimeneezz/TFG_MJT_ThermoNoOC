#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>
#include "Pinout.h"

/**
 * @class Control
 * @brief Manages hardware health: PCB temperature monitoring and cooling fans.
 */
class Control
{
private:
    // PWM Configuration for Fans
    const int fanFreq = 25000; // 25kHz frequency (Industry standard for quiet fans)
    const int fanRes = 8;      // 8-bit resolution (0-255 range)
    const int fanCh = 4;       // PWM Channel 4 (Channels 0-3 reserved for LEDs)

    // Thermistor Constants (B57164K103J)
    const float R0 = 10000.0;     // Nominal resistance (10k Ohms) at 25°C
    const float T0 = 298.15;      // Reference temperature in Kelvin (25°C + 273.15)
    const float BETA = 4300.0;    // Beta coefficient from datasheet
    const float R_FIXED = 4700.0; // Fixed resistor in the voltage divider (4.7k Ohms)

public:
    Control();

    /**
     * Initializes the PWM hardware and ADC resolution.
     */
    void begin();

    /**
     * Reads the NTC thermistor and calculates temperature using the Beta equation.
     * @return Temperature in Celsius.
     */
    float getPCBTemperature();

    /**
     * Sets the duty cycle for the 4 parallel fans.
     * @param speed PWM value from 0 (OFF) to 255 (MAX).
     */
    void setFansSpeed(uint8_t speed);
};

#endif