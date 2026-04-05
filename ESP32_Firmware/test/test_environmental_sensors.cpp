/**
 * @file test_environmental_sensors.cpp
 * @brief Validates SHT35 (Temp/Hum), LTR390 (UV), and T6615 (CO2) sensors.
 */

#include <Arduino.h>
#include "Incubator.h"
#include "Pinout.h"

Incubator incubator;

void setup()
{
    Serial.begin(115200);

    // Initialize incubator sensors (I2C Mux 0x70 + UART2)
    incubator.begin();

    Serial.println("=== ENVIRONMENTAL SENSORS TEST STARTED ===");
    Serial.println("Reading: SHT35 (x2), LTR390 (UV), and T6615 (CO2)");
}

void loop()
{
    // Update all readings
    incubator.readEnvironment();

    Serial.println("--- Environment Report ---");

    // 1. SHT35 Sensors
    Serial.printf("Sensor 1 (Chamber): %.2f C | %.2f %%RH\n", incubator.temp1, incubator.hum1);
    Serial.printf("Sensor 2 (Ambient): %.2f C | %.2f %%RH\n", incubator.temp2, incubator.hum2);

    // 2. UV Sensor
    Serial.printf("UV Index: %.2f\n", incubator.uvIndex);

    // 3. CO2 Sensor
    Serial.printf("CO2 Concentration: %u PPM\n", incubator.co2PPM);

    Serial.println("--------------------------");
    delay(3000); // Wait 3 seconds for next update
}