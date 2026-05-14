#include "Control.h"
#include <math.h>

Control::Control() {}

void Control::begin()
{
    ledcSetup(FAN_PWM_CH, FAN_FREQ_HZ, FAN_RES_BIT);
    ledcAttachPin(PIN_PWM_FANS, FAN_PWM_CH);
    set_Fan_Speed(0);

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
}

int Control::read_NTC_ADC_Average() const
{
    // Per-pin attenuation ensures the full 3.3 V range on PIN_TEMP_PCB
    analogSetPinAttenuation(PIN_TEMP_PCB, ADC_11db);
    long sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++)
        sum += analogRead(PIN_TEMP_PCB);
    return (int)(sum / ADC_SAMPLES);
}

float Control::convert_ADC_To_Temperature(int adcAvg) const
{
    float vFixed = (adcAvg * ADC_VREF) / (float)ADC_MAX;
    float rNtc   = NTC_RFIXED * ((ADC_VREF - vFixed) / vFixed);
    float tempK  = 1.0f / ((1.0f / NTC_T0) + (1.0f / NTC_BETA) * logf(rNtc / NTC_R0));
    return tempK - 273.15f;
}

float Control::read_PCB_Temperature() const
{
    return convert_ADC_To_Temperature(read_NTC_ADC_Average());
}

void Control::set_Fan_Speed(uint8_t speed)
{
    ledcWrite(FAN_PWM_CH, speed);
}

void Control::update_Fan_Speed(float pcbTempC)
{
    float t = (pcbTempC - FAN_TEMP_OFF) / (FAN_TEMP_FULL - FAN_TEMP_OFF);
    t = constrain(t, 0.0f, 1.0f);
    set_Fan_Speed((uint8_t)(t * 255.0f));
}
