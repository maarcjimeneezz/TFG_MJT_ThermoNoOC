#include "LED_Array.h"

LED_Array::LED_Array() {}

void LED_Array::begin()
{
    // Configure LEDC PWM functionalities
    ledcSetup(ledch1, ledFreq, ledRes);
    ledcSetup(ledch2, ledFreq, ledRes);
    ledcSetup(ledch3, ledFreq, ledRes);
    ledcSetup(ledch4, ledFreq, ledRes);

    // Bind the physical GPIO pins (from Pinout.h) to the PWM channels
    ledcAttachPin(PIN_C1_LEDS, ledch1);
    ledcAttachPin(PIN_C2_LEDS, ledch2);
    ledcAttachPin(PIN_C3_LEDS, ledch3);
    ledcAttachPin(PIN_C4_LEDS, ledch4);

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
        ledcWrite(ledch1, dutyCycle);
        break;
    case 2:
        ledcWrite(ledch2, dutyCycle);
        break;
    case 3:
        ledcWrite(ledch3, dutyCycle);
        break;
    case 4:
        ledcWrite(ledch4, dutyCycle);
        break;
    default:
        // Optional: Handle invalid group numbers
        break;
    }
}

void LED_Array::allOff()
{
    ledcWrite(ledch1, 0);
    ledcWrite(ledch2, 0);
    ledcWrite(ledch3, 0);
    ledcWrite(ledch4, 0);
}