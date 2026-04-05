#ifndef INCUBATOR_H
#define INCUBATOR_H

#include <Arduino.h>
#include <Wire.h>
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
    // --- Public Environmental Data ---
    float temp1, hum1; // Data from SHT35 #1
    float temp2, hum2; // Data from SHT35 #2
    float uvIndex;     // Data from LTR390
    uint32_t co2PPM;   // Data from MH-Z19 (CO2)

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
};

#endif