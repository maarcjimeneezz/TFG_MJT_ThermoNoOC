#ifndef INCUBATOR_H
#define INCUBATOR_H

#include <Arduino.h>
#include <Wire.h>
#include "Microfluidics.h"
#include "LED_Array.h"
#include "Pinout.h"
#include <SHTSensor.h>       // SHT3x library
#include <Adafruit_LTR390.h> // UV Sensor library

/**
 * @class Incubator
 * @brief Manages the biological environment: CO2, UV, dual Temperature/Humidity, and ITO heating.
 */
class Incubator
{
private:
    // --- PWM Configuration for ITO Glass ---
    const int itoFreq = 5000; // 5 kHz frequency
    const int itoRes = 8;     // 8-bit resolution (0-255)
    const int itoCh = 5;      // PWM Channel 5 (Channels 0-4 used by LEDs and Fans)

    // --- Sensor Objects ---
    SHTSensor sht1;      // SHT35 Sensor 1
    SHTSensor sht2;      // SHT35 Sensor 2
    Adafruit_LTR390 ltr; // LTR390 UV/Light Sensor

    /**
     * Internal helper to switch the I2C Multiplexer (TCA9548A) channel.
     * @param channel The multiplexer bus to enable (0-7).
     */
    void selectBus(uint8_t channel);

public:
    Microfluidics fluidics; // Manages pumps and flow sensors
    LED_Array leds;         // Manages UV LED array

    // --- Public Environmental Data (Readings) ---
    float temp1, hum1;  // Data from SHT35 #1
    float temp2, hum2;  // Data from SHT35 #2
    float uvIndex;      // Data from LTR390 (0 - 11+)
    float uvIrradiance; // Data from LTR390 (in W/m²)
    float co2Percent;   // Data from T6615 (CO₂ in percentage, 0-20%)
    float flow1, flow2; // Data from SLF3S-0600F (Flow Sensors in µL/min)

    // --- Target Environmental Parameters (UI Commands) ---
    float targetTemperature = 37.0;                   // Desired temperature for the incubator
    bool uvEnabled[4] = {false, false, false, false}; // UV LED group enable states
    int uvIntensity[4] = {0, 0, 0, 0};                // UV LED group intensity (0-255)

    struct PumpConfig
    {
        float flow = 0.0;           // Desired flow rate in µL/min (range: 0-2000)
        String mode = "continuous"; // "continuous" or "pulsed"
        float feedingTime = 0.0;    // For pulsed mode: time to feed in seconds
        float pauseTime = 0.0;      // For pulsed mode: time to pause in seconds
        int cycles = 0;             // For pulsed mode: number of feed/pause cycles
    } pumpConfig[2];                // Configuration for 2 circuits (each with fluid and air pumps)

    Incubator();

    /**
     * Initializes I2C sensors, Serial2 for CO2, and PWM for ITO glass.
     */
    void begin();

    /**
     * Updates all environmental readings from the sensors.
     */
    void readEnvironment();

    /**
     * Sets the heating power for the ITO Glass.
     * @param power PWM duty cycle (0 to 255).
     */
    void setITOPower(uint8_t power);

    /**
     * Updates UV LEDs, ITO heater, and pumps based on the target parameters.
     */
    void updateActuators();
};

#endif