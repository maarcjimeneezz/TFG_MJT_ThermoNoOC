/**
 * @file test_control_ntc_fans.cpp
 * @brief Validates PCB temperature readings and cooling fan PWM control.
 */

#include <Arduino.h>
#include "Control.h"
#include "Pinout.h"

Control control;

void setup()
{
    Serial.begin(115200);
    control.begin();
    Serial.println("=== CONTROL BOARD TEST: NTC & FANS ===");
}

void loop()
{
    // --- 1. Temperature Reading ---
    float pcbTemp = control.getPCBTemperature();
    Serial.printf("Current PCB Temp: %.2f Celsius\n", pcbTemp);

    // --- 2. Fan Speed Test: LOW (approx 30% duty) ---
    Serial.println("Testing Fans: LOW SPEED (80/255)");
    control.setFansSpeed(80);
    delay(4000);

    // --- 3. Fan Speed Test: HIGH (100% duty) ---
    Serial.println("Testing Fans: FULL SPEED (255/255)");
    control.setFansSpeed(255);
    delay(4000);

    // --- 4. Turn OFF ---
    Serial.println("Testing Fans: OFF\n");
    control.setFansSpeed(0);
    delay(2000);
}