/**
 * @file test_control_ntc_fans.cpp
 * @brief Validates PCB temperature readings and cooling fan PWM control.
 */

#include <Arduino.h>
#include "Control.h"
#include "Pinout.h"

Control control_fans;

void setup_control_ntc_fans()
{
    Serial.begin(115200);
    control_fans.begin();
    Serial.println("=== CONTROL BOARD TEST: NTC & FANS ===");
}

void loop_control_ntc_fans()
{
    // Temperature Reading
    float pcbTemp = control_fans.getPCBTemperature();
    Serial.printf("Current PCB Temp: %.2f Celsius\n", pcbTemp);

    // Fan Speed Test: LOW (approx 25% duty)
    Serial.println("Testing Fans: LOW SPEED (64/255)");
    control_fans.setFansSpeed(64);
    delay(2500);

    // Fan Speed Test: LOW-MEDIUM (approx 50% duty)
    Serial.println("Testing Fans: LOW-MEDIUM SPEED (128/255)");
    control_fans.setFansSpeed(128);
    delay(2500);

    // Fan Speed Test: MEDIUM-HIGH (approx 75% duty)
    Serial.println("Testing Fans: MEDIUM-HIGH SPEED (192/255)");
    control_fans.setFansSpeed(192);
    delay(2500);

    // Fan Speed Test: HIGH (100% duty)
    Serial.println("Testing Fans: FULL SPEED (255/255)");
    control_fans.setFansSpeed(255);
    delay(2500);

    // Turn OFF
    Serial.println("Testing Fans: OFF\n");
    control_fans.setFansSpeed(0);
    delay(2500);
}