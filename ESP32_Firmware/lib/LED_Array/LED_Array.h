#ifndef LED_ARRAY_H
#define LED_ARRAY_H

#include <Arduino.h>
#include "Pinout.h"

/**
 * @class LED_Array
 * @brief Manages the UV LED array through 4 independent PWM channels.
 */
class LED_Array
{
private:
    // --- PWM Configuration ---
    const int ledFreq = 5000; // 5 kHz to avoid flickering
    const int ledRes = 8;     // 8-bit (0-255) for easy LabVIEW mapping

    // --- PWM Channels (ESP32 has 16 independent channels 0-15) ---
    const int ledch1 = 0;
    const int ledch2 = 1;
    const int ledch3 = 2;
    const int ledch4 = 3;

public:
    LED_Array();

    /**
     * Initializes the PWM hardware for all LED channels.
     */
    void begin();

    /**
     * Sets brightness for a specific LED group independently.
     * @param group The LED group number (1 to 4).
     * @param value PWM duty cycle (0 to 255).
     */
    void setBrightness(int group, int value);

    /**
     * Turns off all LED groups simultaneously.
     */
    void allOff();
};

#endif