/**
 * @file test_first_pcb.cpp
 * @brief Basic validation: Built-in LED blink and PCB Temperature reading.
 */

#include <Arduino.h>
#include "Control.h"
#include "Pinout.h"

// Define the onboard LED pin.
#define ONBOARD_LED 2

Control control_pcb;

void setup_first_pcb()
{
    // Start Serial for debugging
    Serial.begin(115200);
    delay(1000);

    Serial.println("PCB TEST 01: HEARTBEAT & THERMAL  ");

    // Initialize the Control library (Setup ADC for NTC)
    control_pcb.begin();

    // Configure the LED pin
    pinMode(ONBOARD_LED, OUTPUT);

    Serial.println("Setup Complete. Starting loop...");
}

void loop_first_pcb()
{
    // Heartbeat (Blink)
    digitalWrite(ONBOARD_LED, HIGH);
    delay(500);
    digitalWrite(ONBOARD_LED, LOW);

    // Thermal Reading
    // This uses your Control library logic which likely uses the Steinhart-Hart equation
    float currentTemp = control_pcb.getPCBTemperature();

    Serial.print("[INFO] Heartbeat OK | PCB Temperature: ");
    Serial.print(currentTemp);
    Serial.println(" °C");

    // Wait 1 second before next reading
    delay(1000);
}