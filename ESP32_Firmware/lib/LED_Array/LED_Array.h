#ifndef LED_ARRAY_H
#define LED_ARRAY_H

#include <Arduino.h>
#include "Pinout.h"

class LED_Array
{
private:
    // PWM Configuration
    const int ledFreq = 5000; // 5 kHz to avoid flickering
    const int ledRes = 8;     // 8-bit (0-255) for easy LabVIEW mapping

    // PWM Channels (ESP32 has 16 independent channels 0-15)
    const int ledch1 = 0;
    const int ledch2 = 1;
    const int ledch3 = 2;
    const int ledch4 = 3;

public:
    LED_Array();
    void begin();

    /**
     * Set brightness for a specific group independently.
     * @param group The LED group number (1 to 4)
     * @param value PWM duty cycle (0 to 255)
     */
    void setBrightness(int group, int value);

    void allOff();
};

#endif