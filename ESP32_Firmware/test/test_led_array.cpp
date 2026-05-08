/**
 * @file test_led_array.cpp
 * @brief Independently fades each of the 4 LED groups to verify PWM and MOSFETs.
 */

#include <Arduino.h>
#include "LED_Array.h"

LED_Array leds;

void setup_led_array()
{
    Serial.begin(115200);
    leds.begin();
    Serial.println("=== LED ARRAY TEST: INDIVIDUAL CHANNELS ===");
}

void loop_led_array()
{
    for (int group = 1; group <= 4; group++)
    {
        Serial.printf("Testing LED Group %d: Fading Up...\n", group);

        // Fade Up from 0 to 255 in steps of 5 for smooth transition
        for (int brightness = 0; brightness <= 255; brightness += 5)
        {
            leds.setBrightness(group, brightness);
            delay(20);
        }

        delay(1000); // Hold at max brightness

        Serial.printf("Testing LED Group %d: Fading Down...\n", group);

        // Fade Down from 255 to 0 in steps of 5 for smooth transition
        for (int brightness = 255; brightness >= 0; brightness -= 5)
        {
            leds.setBrightness(group, brightness);
            delay(20);
        }

        leds.allOff();
        delay(500);
    }
    Serial.println("Full LED Array cycle completed.\n");
}