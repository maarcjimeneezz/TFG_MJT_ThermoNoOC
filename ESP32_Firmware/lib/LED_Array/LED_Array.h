#ifndef LED_ARRAY_H
#define LED_ARRAY_H

#include <Arduino.h>
#include "Pinout.h"

class LED_Array
{
private:
    // PWM Configuration
    const int freq = 5000;    // 5 kHz to avoid flickering
    const int resolution = 8; // 8-bit (0-255) for easy LabVIEW mapping

    // PWM Channels (ESP32 has 16 independent channels 0-15)
    const int ch1 = 0;
    const int ch2 = 1;
    const int ch3 = 2;
    const int ch4 = 3;

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