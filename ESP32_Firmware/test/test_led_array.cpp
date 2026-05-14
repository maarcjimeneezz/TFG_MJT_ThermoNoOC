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

        leds.set_Group_Enabled(group, true);

        // Fade Up from 0 to 100 % in steps of 5 for smooth transition
        for (int intensity = 0; intensity <= 100; intensity += 5)
        {
            leds.set_Group_Intensity(group, (uint8_t)intensity);
            leds.update_All_Groups();
            delay(20);
        }

        delay(1000); // Hold at max brightness

        Serial.printf("Testing LED Group %d: Fading Down...\n", group);

        // Fade Down from 100 to 0 % in steps of 5 for smooth transition
        for (int intensity = 100; intensity >= 0; intensity -= 5)
        {
            leds.set_Group_Intensity(group, (uint8_t)intensity);
            leds.update_All_Groups();
            delay(20);
        }

        leds.all_Off();
        delay(500);
    }
    Serial.println("Full LED Array cycle completed.\n");
}