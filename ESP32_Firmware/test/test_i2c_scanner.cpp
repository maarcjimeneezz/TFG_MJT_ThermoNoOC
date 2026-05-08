/**
 * @file test_i2c_scanner.cpp
 * @brief Deep scan of all I2C channels behind the 0x70 and 0x71 multiplexers.
 */

#include <Arduino.h>
#include <Wire.h>
#include "Pinout.h"

/**
 * Opens a specific channel on a TCA9548A multiplexer and scans for devices.
 * @param muxAddr I2C address of the multiplexer (0x70 or 0x71).
 * @param muxName Label for serial debugging.
 */
void scanMultiplexer(uint8_t muxAddr, const char *muxName)
{
    Serial.printf("\n--- Scanning %s Multiplexer (0x%02X) ---\n", muxName, muxAddr);

    for (uint8_t channel = 0; channel < 8; channel++)
    {
        // Select the multiplexer channel
        Wire.beginTransmission(muxAddr);
        Wire.write(1 << channel);
        if (Wire.endTransmission() != 0)
        {
            Serial.printf("Channel %d: Multiplexer not responding\n", channel);
            continue;
        }

        // Scan all possible I2C addresses on this channel
        Serial.printf("Channel %d: [ ", channel);
        bool found = false;
        for (uint8_t addr = 1; addr < 127; addr++)
        {
            // Skip the multiplexers themselves to avoid confusion
            if (addr == 0x70 || addr == 0x71)
                continue;

            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0)
            {
                Serial.printf("0x%02X ", addr);
                found = true;
            }
        }
        if (!found)
            Serial.print("Empty ");
        Serial.println("]");
    }
}

void setup_i2c_scanner()
{
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(2000);
    Serial.println("I2C HARDWARE VERIFICATION STARTED");
}

void loop_i2c_scanner()
{
    // Perform scan every 5 seconds
    scanMultiplexer(MUX_ADDR_SENSORS, "SENSORS");
    scanMultiplexer(MUX_ADDR_MICROFLUIDICS, "FLUIDICS");

    Serial.println("\nScan finished. Waiting 5s...");
    delay(5000);
}