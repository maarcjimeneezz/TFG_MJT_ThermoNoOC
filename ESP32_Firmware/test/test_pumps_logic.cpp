/**
 * @file test_pumps_logic.cpp
 * @brief Stress test: Runs all 4 pumps simultaneously and monitors flow.
 * Logic:
 * - Circuit 1: Pumps 1 (fluid) & 3 (air) at 80 Hz
 * - Circuit 2: Pumps 2 (fluid) & 4 (air) at 300 Hz
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
    fluidics.set_Pump_Frequency(1, 80);
    fluidics.set_Pump_Voltage(1, 180); // High amplitude to test power stability

    fluidics.set_Pump_Frequency(3, 80);
    fluidics.set_Pump_Voltage(3, 180);

    // Air path (Bubble trap)
    fluidics.set_Pump_Frequency(2, 300);
    fluidics.set_Pump_Voltage(2, 220); // Air usually needs more "punch"

    fluidics.set_Pump_Frequency(4, 300);
    fluidics.set_Pump_Voltage(4, 220);

    Serial.println("All pumps are running. Monitoring flow sensors...");
}

void loop_pumps_logic()
{
    // Read both flow sensors while pumps are active
    float f1 = fluidics.read_Flow_Rate(1);
    float f2 = fluidics.read_Flow_Rate(2);

    Serial.print("Flow Readings -> Sensor 1: ");
    Serial.print(f1);
    Serial.print(" ml/min | Sensor 2: ");
    Serial.print(f2);
    Serial.println(" ml/min");

    // Heat check: In your TFG, mention that running all 4 pumps
    // at max voltage is the worst-case scenario for PCB heat.
    delay(1000);
}