#include "Incubator.h"

// Constructor: Initializing SHT objects as SHT3x type
Incubator::Incubator() : sht1(SHTSensor::SHT3X), sht2(SHTSensor::SHT3X)
{
    temp1 = hum1 = temp2 = hum2 = uvIndex = 0.0f;
    co2PPM = 0;
}

void Incubator::selectBus(uint8_t channel)
{
    // MUX_ADDR_SENSORS is 0x70
    Wire.beginTransmission(MUX_ADDR_SENSORS);
    Wire.write(1 << channel);
    Wire.endTransmission();
}

void Incubator::begin()
{
    // --- Initialize Serial2 for CO2 Sensor (9600 baud) ---
    Serial2.begin(9600, SERIAL_8N1, PIN_CO2_RX, PIN_CO2_TX);

    // --- Initialize ITO Glass PWM ---
    ledcSetup(itoCh, itoFreq, itoRes);
    ledcAttachPin(PIN_ITO_CONTROL, itoCh);
    setITOPower(0);

    // --- Initialize I2C Sensors through the Multiplexer ---

    // Sensor 1 (SHT35)
    selectBus(MUX_CH_TEMP1);
    sht1.init();

    // Sensor 2 (SHT35)
    selectBus(MUX_CH_TEMP2);
    sht2.init();

    // UV Sensor (LTR390)
    selectBus(MUX_CH_UV);
    if (!ltr.begin())
    {
        // Optional: Serial.println("Failed to find LTR390");
    }
    ltr.setMode(LTR390_MODE_UVS); // Set to UV sensing mode
}

void Incubator::readEnvironment()
{
    // 1. Read SHT35 - Channel 1
    selectBus(MUX_CH_TEMP1);
    if (sht1.readSample())
    {
        temp1 = sht1.getTemperature();
        hum1 = sht1.getHumidity();
    }

    // 2. Read SHT35 - Channel 2
    selectBus(MUX_CH_TEMP2);
    if (sht2.readSample())
    {
        temp2 = sht2.getTemperature();
        hum2 = sht2.getHumidity();
    }

    // 3. Read UV Sensor
    selectBus(MUX_CH_UV);
    if (ltr.newDataAvailable())
    {
        uvIndex = ltr.readUVI();
    }

    // 4. Read CO2 Sensor via Serial2
    // Basic logic for MH-Z19 (sending 9-byte command)
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    Serial2.write(cmd, 9);

    uint8_t response[9];
    if (Serial2.available() > 0)
    {
        Serial2.readBytes(response, 9);
        if (response[0] == 0xFF && response[1] == 0x86)
        {
            co2PPM = (response[2] << 8) + response[3];
        }
    }
}

void Incubator::setITOPower(uint8_t power)
{
    // Apply PWM to the ITO control pin
    ledcWrite(itoCh, power);
}