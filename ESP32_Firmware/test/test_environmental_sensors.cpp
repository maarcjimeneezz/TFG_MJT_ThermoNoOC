/**
 * @file test_environmental_sensors.cpp
 * @brief Validates SHT35 (Temp/Hum), LTR390 (UV), and T6615 (CO2) sensors.
 */

#include <Arduino.h>
#include "Incubator.h"
#include "Pinout.h"

Incubator incubator;

void setup_environmental_sensors()
{
    Serial.begin(115200);
    delay(2000); // Allow time for serial monitor to connect

    // Initialize incubator sensors (I2C Mux 0x70 + UART2)
    incubator.begin();

    Serial.println("=== ENVIRONMENTAL SENSORS TEST STARTED ===");
    Serial.println("Waiting 5s for CO2 sensor stabilization (Warm-up)...");
    delay(5000);

    Serial.println("Reading: SHT35 (x2), LTR390 (UV), and T6615 (CO2)");
}

void loop_environmental_sensors()
{
    // Update all readings
    incubator.readEnvironment();

    Serial.println("--- Environment Report ---");

    // SHT35 Sensors
    Serial.printf("Sensor 1 (Glass): %.2f C | %.2f %%RH\n", incubator.temp1, incubator.hum1);
    Serial.printf("Sensor 2 (Base): %.2f C | %.2f %%RH\n", incubator.temp2, incubator.hum2);

    // UV Sensor
    Serial.printf("UV Index: %.2f\n", incubator.uvIndex);

    // CO2 Sensor
    Serial.printf("CO2 Concentration: %.4f %%\n", incubator.co2Percent);

    Serial.println("--------------------------");
    delay(3000); // Wait 3 seconds for next update
}