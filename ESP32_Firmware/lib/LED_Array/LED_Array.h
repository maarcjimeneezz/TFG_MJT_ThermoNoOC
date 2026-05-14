#ifndef LED_ARRAY_H
#define LED_ARRAY_H

#include <Arduino.h>
#include "Pinout.h"

/**
 * @class LED_Array
 * @brief Manages the 4-group UV LED matrix via independent LEDC PWM channels.
 *
 * Single responsibility: LED state (enabled/intensity) and PWM output.
 * Callers set state via set_Group_Enabled / set_Group_Intensity and call
 * update_All_Groups() each loop tick to apply changes to hardware.
 * No knowledge of incubator targets, pumps, WiFi, or sensors.
 */
class LED_Array
{
private:
    static const int NUM_GROUPS = 4;
    static const int     LED_FREQ_HZ = 5000; // 5 kHz eliminates visible flicker
    static const int     LED_RES_BIT = 8;   // 0-255 duty cycle range
    static const uint8_t LED_PWM_MIN = 128; // 50 % floor — LEDs need this to actually light up

    // Defined in .cpp to keep Pinout.h macros out of the header
    static const int CHANNELS[NUM_GROUPS]; // LEDC channels 0-3
    static const int PINS[NUM_GROUPS];     // GPIO pins from Pinout.h

    bool _enabled[NUM_GROUPS];
    uint8_t _intensity[NUM_GROUPS];

    void apply_PWM_To_Group(int idx);

public:
    LED_Array();

    /** Configures all 4 LEDC channels and sets LEDs to OFF. Call once in setup(). */
    void begin();

    /** Enables or disables group (1-4). Takes effect on next update_All_Groups(). */
    void set_Group_Enabled(int group, bool enabled);

    /** Sets brightness (0-255) for group (1-4). Takes effect on next update_All_Groups(). */
    void set_Group_Intensity(int group, uint8_t intensity);

    /** Applies current enabled/intensity state to all PWM channels. Call every loop(). */
    void update_All_Groups();

    /** Immediately drives all channels to 0 and clears enabled/intensity state. */
    void all_Off();
};

#endif // LED_ARRAY_H