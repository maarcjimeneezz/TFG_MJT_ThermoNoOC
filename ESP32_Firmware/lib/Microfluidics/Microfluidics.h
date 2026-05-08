#ifndef MICROFLUIDICS_H
#define MICROFLUIDICS_H

#include <Arduino.h>
#include <Wire.h>
#include "Pinout.h"

/**
 * @class Microfluidics
 * @brief Manages 4 Bartels mp-Lowdrivers and 2 Sensirion SLF3S flow sensors via I2C Mux 0x71.
 */
class Microfluidics
{
private:
    // --- Device I2C Addresses ---
    const uint8_t pumpAddr = 0x59; // mp-Lowdriver default I2C address
    const uint8_t flowAddr = 0x08; // SLF3S-0600F default I2C address

    /**
     * Internal helper to switch the Microfluidics I2C Multiplexer (0x71) channel.
     * @param channel The multiplexer bus to enable (0-7).
     */
    void selectBus(uint8_t channel);

    /**
     * Switches the mp-Lowdriver internal register page.
     * @param page 0x00 for Control, 0x01 for Memory.
     */
    void setPumpPage(uint8_t page);

public:
    Microfluidics();

    /**
     * Initializes all 4 pumps and 2 flow sensors using the channels defined in Pinout.h.
     */
    void begin();

    /**
     * Reads flow rate from a specific sensor.
     * @param sensorNum Sensor index (1 or 2).
     * @return Flow rate in ml/min.
     */
    float getFlowRate(int sensorNum);

    /**
     * Updates voltage for a specific pump.
     * @param pumpNum Pump index (1 to 4).
     * @param voltage 8-bit value (0-255) mapping to amplitude.
     */
    void setPumpVoltage(int pumpNum, uint8_t voltage);

    /**
     * Updates frequency for a specific pump.
     * @param pumpNum Pump index (1 to 4).
     * @param frequency Target frequency in Hz.
     */
    void setPumpFrequency(int pumpNum, uint16_t frequency);
};

#endif