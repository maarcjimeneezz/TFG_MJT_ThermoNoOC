/**
 * @file test_pumps_parallel.cpp
 * @brief Stress test: Runs all 4 pumps simultaneously and monitors flow.
 * * Logic:
 * - Pumps 1 & 3: Liquid (80 Hz)
 * - Pumps 2 & 4: Air/Bubble Trap (300 Hz)
 */

#include <Arduino.h>
#include "Microfluidics.h"
#include "Pinout.h"

Microfluidics fluidics;

void setup_pumps_logic()
{
    Serial.begin(115200);

    // Initializing the I2C bus and the mp-Lowdrivers
    fluidics.begin();

    Serial.println("=== PARALLEL PUMP STRESS TEST ===");
    Serial.println("Action: Starting all 4 pumps... NOW.");

    // START ALL PUMPS SIMULTANEOUSLY
    // Note: The Mux switches channels fast enough that they seem to start at once.

    // Liquid path
    fluidics.setPumpFrequency(1, 80);
    fluidics.setPumpVoltage(1, 180); // High amplitude to test power stability

    fluidics.setPumpFrequency(3, 80);
    fluidics.setPumpVoltage(3, 180);

    // Air path (Bubble trap)
    fluidics.setPumpFrequency(2, 300);
    fluidics.setPumpVoltage(2, 220); // Air usually needs more "punch"

    fluidics.setPumpFrequency(4, 300);
    fluidics.setPumpVoltage(4, 220);

    Serial.println("All pumps are running. Monitoring flow sensors...");
}

void loop_pumps_logic()
{
    // Read both flow sensors while pumps are active
    float f1 = fluidics.getFlowRate(1);
    float f2 = fluidics.getFlowRate(2);

    Serial.print("Flow Readings -> Sensor 1: ");
    Serial.print(f1);
    Serial.print(" ml/min | Sensor 2: ");
    Serial.print(f2);
    Serial.println(" ml/min");

    // Heat check: In your TFG, mention that running all 4 pumps
    // at max voltage is the worst-case scenario for PCB heat.
    delay(1000);
}