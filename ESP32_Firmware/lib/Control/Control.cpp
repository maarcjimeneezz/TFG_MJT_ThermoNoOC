#include "Control.h"
#include <math.h>

Control::Control() {}

void Control::begin()
{
    // --- Initialize ESP32 LEDC PWM peripheral for fans ---
    ledcSetup(fanCh, fanFreq, fanRes);
    ledcAttachPin(PIN_PWM_FANS, fanCh);

    // --- Ensure fans are off at startup ---
    setFansSpeed(0);

    // --- Set ADC resolution to 12-bit (0-4095) for better precision ---
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db); // Set attenuation for full 3.3V range on ADC
}

float Control::getPCBTemperature()
{
    analogReadResolution(12);                        // Ensure ADC is set to 12-bit resolution
    analogSetPinAttenuation(PIN_TEMP_PCB, ADC_11db); // Set attenuation for the NTC pin to read up to 3.3V

    // Average multiple readings to reduce noise
    long sum = 0;
    for (int i = 0; i < 15; i++)
    {
        sum += analogRead(PIN_TEMP_PCB);
        delay(1); // Short delay between readings
    }
    int adcAvg = sum / 15; // Use the average of the 15 readings

    // Voltage Calculation
    float vFixed = (adcAvg * 3.3) / 4095.0; // Convert ADC value to voltage (assuming 3.3V reference)

    // Resistance Calculation
    float rNtc = R_FIXED * ((3.3 - vFixed) / vFixed); // Calculate NTC resistance using voltage divider formula

    // Beta Equation to calculate temperature in Kelvin
    float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(rNtc / R0));

    // Convert Kelvin to Celsius
    float tempC = tempK - 273.15;

    Serial.printf("NTC ADC: %d, Voltage: %.2f V, R_ntc: %.2f Ohms, Temp: %.2f °C\n", adcAvg, vFixed, rNtc, tempC);

    return tempC;
}

void Control::setFansSpeed(uint8_t speed)
{
    // Since all 4 fans share the same pin, this controls them simultaneously
    ledcWrite(fanCh, speed);
}