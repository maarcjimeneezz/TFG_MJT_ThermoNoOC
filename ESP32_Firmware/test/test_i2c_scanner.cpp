/**
 * @file test_i2c_scanner.cpp
 * @brief Deep scan of all I2C channels behind the 0x70 and 0x71 multiplexers.
 */

#include <Arduino.h>
#include <Wire.h>
#include "Pinout.h"

/**
 * Scans the raw I2C bus (no mux) to verify basic bus health before mux scanning.
 */
static void scanDirectBus()
{
    Serial.println("\n--- Scanning direct I2C bus (no mux) ---");
    bool found = false;
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            Serial.printf("  Found device at 0x%02X\n", addr);
            found = true;
        }
    }
    if (!found)
        Serial.println("  No devices found. Check SDA/SCL wiring and pull-ups.");
}

/**
 * Deselects all channels on a TCA9548A by writing 0x00 to it.
 * Call after finishing a channel scan to prevent bus contention on the next scan.
 */
static void deselectMux(uint8_t muxAddr)
{
    Wire.beginTransmission(muxAddr);
    Wire.write(0x00);
    Wire.endTransmission();
}

/**
 * Opens a specific channel on a TCA9548A multiplexer and scans for devices.
 * @param muxAddr  I2C address of the multiplexer (0x70 or 0x71).
 * @param muxName  Label for serial debugging.
 */
static void scanMultiplexer(uint8_t muxAddr, const char *muxName)
{
    Serial.printf("\n--- Scanning %s Multiplexer (0x%02X) ---\n", muxName, muxAddr);

    // Deselect all channels before starting to ensure a clean state
    deselectMux(muxAddr);

    for (uint8_t channel = 0; channel < 8; channel++)
    {
        // Select the multiplexer channel
        Wire.beginTransmission(muxAddr);
        Wire.write(1 << channel);
        uint8_t err = Wire.endTransmission();

        if (err != 0)
        {
            // err==2: NACK on address (mux not on bus)
            // err==3: NACK on data  (mux rejected the channel byte)
            // err==4: bus error     (SDA/SCL stuck)
            // err==5: timeout
            Serial.printf("  Channel %d: Mux not responding (err=%d)\n", channel, err);
            // If the mux itself is missing there is no point continuing this mux
            if (err == 2)
            {
                Serial.printf("  -> Mux 0x%02X not found on bus. Stopping scan.\n", muxAddr);
                return;
            }
            continue;
        }

        // Brief settle time after channel switch (TCA9548A datasheet: ~1 µs, but
        // stray capacitance on long wires may need more)
        delay(1);

        // Scan all possible I2C addresses on this channel
        Serial.printf("  Channel %d: [ ", channel);
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

        // Deselect this channel before moving to the next one to avoid bus
        // contention from devices that remain active on the selected channel
        deselectMux(muxAddr);
    }
}

void setup_i2c_scanner()
{
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(2000);
    Serial.println("=== I2C HARDWARE VERIFICATION STARTED ===");
    Serial.printf("SDA=GPIO%d  SCL=GPIO%d\n", I2C_SDA, I2C_SCL);
}

void loop_i2c_scanner()
{
    // Step 1: verify raw bus before going through mux
    scanDirectBus();

    // Step 2: scan through each multiplexer
    scanMultiplexer(MUX_ADDR_SENSORS, "SENSORS");
    scanMultiplexer(MUX_ADDR_MICROFLUIDICS, "FLUIDICS");

    Serial.println("\nScan finished. Waiting 5s...");
    delay(5000);
}
