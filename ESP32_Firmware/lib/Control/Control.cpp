#include "Control.h"
#include <math.h>

Control::Control() {}

void Control::begin()
{
    // Initialize ESP32 LEDC PWM peripheral for fans
    ledcSetup(fanCh, fanFreq, fanRes);
    ledcAttachPin(PIN_PWM_FANS, fanCh);

    // Ensure fans are off at startup
    setFansSpeed(0);

    // Set ADC resolution to 12-bit (0-4095) for better precision
    analogReadResolution(12);
}

float Control::getPCBTemperature()
{
    // Read the raw analog value from the NTC divider pin
    int adcVal = analogRead(PIN_TEMP_PCB);

    // Boundary check to prevent division by zero or log errors
    if (adcVal <= 0 || adcVal >= 4095)
        return -273.15;

    /* * 1. Calculate NTC Resistance
     * Voltage Divider Circuit: 3.3V -> R_FIXED (4.7k) -> PIN -> NTC -> GND
     * formula: R_ntc = R_fixed * (V_out / (V_source - V_out))
     */
    float vOut = (adcVal * 3.3) / 4095.0;
    float rNtc = R_FIXED * (vOut / (3.3 - vOut));

    /* * 2. Apply the Beta Equation: 1/T = 1/T0 + 1/B * ln(R/R0)
     * This provides the temperature in Kelvin.
     */
    float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(rNtc / R0));

    // Convert Kelvin to Celsius
    return tempK - 273.15;
}

void Control::setFansSpeed(uint8_t speed)
{
    // Since all 4 fans share the same pin, this controls them simultaneously
    ledcWrite(fanCh, speed);
}