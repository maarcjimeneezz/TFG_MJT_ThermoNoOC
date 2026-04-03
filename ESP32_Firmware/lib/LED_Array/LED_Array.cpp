#include "LED_Array.h"

LED_Array::LED_Array() {}

void LED_Array::begin()
{
    // Configure LEDC PWM functionalities
    ledcSetup(ch1, freq, resolution);
    ledcSetup(ch2, freq, resolution);
    ledcSetup(ch3, freq, resolution);
    ledcSetup(ch4, freq, resolution);

    // Bind the physical GPIO pins (from Pinout.h) to the PWM channels
    ledcAttachPin(PIN_C1_LEDS, ch1);
    ledcAttachPin(PIN_C2_LEDS, ch2);
    ledcAttachPin(PIN_C3_LEDS, ch3);
    ledcAttachPin(PIN_C4_LEDS, ch4);

    // Initialize all LEDs to 0 (OFF)
    allOff();
}

void LED_Array::setBrightness(int group, int value)
{
    // Ensure the value stays within the 8-bit range
    uint8_t dutyCycle = (uint8_t)constrain(value, 0, 255);

    switch (group)
    {
    case 1:
        ledcWrite(ch1, dutyCycle);
        break;
    case 2:
        ledcWrite(ch2, dutyCycle);
        break;
    case 3:
        ledcWrite(ch3, dutyCycle);
        break;
    case 4:
        ledcWrite(ch4, dutyCycle);
        break;
    default:
        // Optional: Handle invalid group numbers
        break;
    }
}

void LED_Array::allOff()
{
    ledcWrite(ch1, 0);
    ledcWrite(ch2, 0);
    ledcWrite(ch3, 0);
    ledcWrite(ch4, 0);
}