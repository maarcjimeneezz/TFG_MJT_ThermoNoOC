#include "LED_Array.h"

const int LED_Array::CHANNELS[4] = {0, 1, 2, 3};
const int LED_Array::PINS[4] = {PIN_C1_LEDS, PIN_C2_LEDS, PIN_C3_LEDS, PIN_C4_LEDS};

LED_Array::LED_Array()
{
    for (int i = 0; i < NUM_GROUPS; i++)
    {
        _enabled[i] = false;
        _intensity[i] = 0;
    }
}

void LED_Array::begin()
{
    for (int i = 0; i < NUM_GROUPS; i++)
    {
        ledcSetup(CHANNELS[i], LED_FREQ_HZ, LED_RES_BIT);
        ledcAttachPin(PINS[i], CHANNELS[i]);
    }
    all_Off();
}

void LED_Array::apply_PWM_To_Group(int idx)
{
    uint8_t pwm = 0;
    if (_enabled[idx] && _intensity[idx] > 0)
        pwm = LED_PWM_MIN + (_intensity[idx] * (255 - LED_PWM_MIN)) / 100;
    ledcWrite(CHANNELS[idx], pwm);
}

void LED_Array::set_Group_Enabled(int group, bool enabled)
{
    if (group < 1 || group > NUM_GROUPS)
        return;
    _enabled[group - 1] = enabled;
}

void LED_Array::set_Group_Intensity(int group, uint8_t intensity)
{
    if (group < 1 || group > NUM_GROUPS)
        return;
    _intensity[group - 1] = intensity;
}

void LED_Array::update_All_Groups()
{
    for (int i = 0; i < NUM_GROUPS; i++)
        apply_PWM_To_Group(i);
}

void LED_Array::all_Off()
{
    for (int i = 0; i < NUM_GROUPS; i++)
    {
        _enabled[i] = false;
        ledcWrite(CHANNELS[i], 0);
    }
}