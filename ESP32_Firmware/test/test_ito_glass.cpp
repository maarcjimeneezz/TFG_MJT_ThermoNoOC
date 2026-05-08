/**
 * @file test_ito_glass.cpp
 * @brief Thermal test for ITO Glass heating using PWM control.
 * * Matches the configuration used in the Incubator class (5kHz, Channel 5).
 */

#include <Arduino.h>
#include "Pinout.h"

// PWM Configuration
const int itoFreq = 5000; // 5 kHz frequency
const int itoRes = 8;     // 8-bit resolution (0-255)
const int itoCh = 5;      // PWM Channel 5

// Constants for Power Estimation
const float V_SOURCE = 24.0; // Power supply voltage
const float R_APPROX = 15.0; // Measured resistance of your ITO

/**
 * Initializes the PWM hardware for the ITO Control pin.
 */
void setup_ito_glass()
{
    Serial.begin(115200);

    // Initialize ESP32 PWM peripheral
    ledcSetup(itoCh, itoFreq, itoRes);
    ledcAttachPin(PIN_ITO_CONTROL, itoCh);

    // Ensure it starts OFF
    ledcWrite(itoCh, 0);

    Serial.println("ITO GLASS THERMAL TEST (5 kHz)");
    Serial.println("Enter power percentage (0-100):");
}

/**
 * Updates heating power based on Serial Monitor input.
 */
void loop_ito_glass()
{
    if (Serial.available() > 0)
    {
        int powerPct = Serial.parseInt();

        // Clear buffer
        while (Serial.available() > 0)
            Serial.read();

        if (powerPct >= 0 && powerPct <= 100)
        {
            // Map 0-100% to 0-255 PWM
            int pwmValue = map(powerPct, 0, 100, 0, 255);
            ledcWrite(itoCh, pwmValue);

            // Calculation for monitoring
            float currentPower = ((V_SOURCE * V_SOURCE) / R_APPROX) * (powerPct / 100.0);

            Serial.printf("[UPDATE] Power: %d%% | PWM: %d | Est: %.2f W\n",
                          powerPct, pwmValue, currentPower);
        }
        else
        {
            Serial.println("[ERROR] Value must be between 0 and 100.");
        }
    }
    delay(100);
}