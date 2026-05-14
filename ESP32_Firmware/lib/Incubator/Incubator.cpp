#include "Incubator.h"

Incubator::Incubator()
    : _sht1(SHTSensor::AUTO_DETECT),
      _sht2(SHTSensor::AUTO_DETECT),
      _co2State(CO2State::IDLE),
      _co2CmdTime(0),
      temp1(0.0f), hum1(0.0f),
      temp2(0.0f), hum2(0.0f),
      uvIndex(0.0f), uvIrradiance(0.0f),
      co2Percent(0.0f),
      targetTemperature(37.0f) {}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Incubator::select_Sensor_Bus(uint8_t muxChannel)
{
    Wire.beginTransmission(MUX_ADDR_SENSORS);
    Wire.write(1 << muxChannel);
    Wire.endTransmission();
}

void Incubator::read_SHT35_Sensors()
{
    select_Sensor_Bus(MUX_CH_TEMP1);
    if (_sht1.readSample())
    {
        temp1 = _sht1.getTemperature();
        hum1 = _sht1.getHumidity();
    }
    else
    {
        temp1 = hum1 = 0.0f;
        Serial.println("[Incubator] WARN: SHT35 #1 read failed");
    }

    select_Sensor_Bus(MUX_CH_TEMP2);
    if (_sht2.readSample())
    {
        temp2 = _sht2.getTemperature();
        hum2 = _sht2.getHumidity();
    }
    else
    {
        temp2 = hum2 = 0.0f;
        Serial.println("[Incubator] WARN: SHT35 #2 read failed");
    }
}

void Incubator::read_UV_Sensor()
{
    select_Sensor_Bus(MUX_CH_UV);
    if (_ltr.newDataAvailable())
    {
        uint32_t raw = _ltr.readUVS();
        uvIndex = (float)raw / LTR390_SENSITIVITY;
        uvIrradiance = uvIndex * LTR390_UVI_TO_WM2;
    }
    // If no new data, retain last valid reading until the next integration cycle
}

void Incubator::tick_CO2_State_Machine()
{
    switch (_co2State)
    {
    case CO2State::IDLE:
    {
        // Flush stale bytes then issue a read command (T6615 manual p.8: FF FE 02 02 03)
        while (Serial2.available())
            Serial2.read();
        const uint8_t cmd[] = {0xFF, 0xFE, 0x02, 0x02, 0x03};
        Serial2.write(cmd, sizeof(cmd));
        _co2CmdTime = millis();
        _co2State = CO2State::CMD_SENT;
        break;
    }
    case CO2State::CMD_SENT:
    {
        if (Serial2.available() >= 5)
        {
            uint8_t resp[5];
            Serial2.readBytes(resp, 5);
            if (resp[0] == 0xFF && resp[1] == 0xFA)
            {
                // T6615 manual p.8: CO2 value in bytes 3-4 (MSB first)
                uint16_t raw = ((uint16_t)resp[3] << 8) | resp[4];
                co2Percent = (float)raw / 10000.0f;
            }
            else
            {
                Serial.printf("[Incubator] CO2 frame error: 0x%02X 0x%02X\n", resp[0], resp[1]);
            }
            _co2State = CO2State::IDLE;
        }
        else if (millis() - _co2CmdTime > CO2_TIMEOUT_MS)
        {
            Serial.println("[Incubator] CO2 timeout — retrying next cycle");
            _co2State = CO2State::IDLE;
        }
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Incubator::begin()
{
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial2.begin(19200, SERIAL_8N1, PIN_CO2_RX, PIN_CO2_TX);

    ledcSetup(ITO_PWM_CH, ITO_FREQ_HZ, ITO_RES_BIT);
    ledcAttachPin(PIN_ITO_CONTROL, ITO_PWM_CH);
    set_ITO_Power(0);

    select_Sensor_Bus(MUX_CH_TEMP1);
    delay(50); // SHT35 power-on stabilisation (datasheet min: 1 ms; 50 ms for safety margin)
    if (!_sht1.init())
        Serial.println("[Incubator] ERROR: SHT35 #1 not found");
    else
        Serial.println("[Incubator] SHT35 #1 OK");

    select_Sensor_Bus(MUX_CH_TEMP2);
    delay(50);
    if (!_sht2.init())
        Serial.println("[Incubator] ERROR: SHT35 #2 not found");
    else
        Serial.println("[Incubator] SHT35 #2 OK");

    select_Sensor_Bus(MUX_CH_UV);
    if (!_ltr.begin())
    {
        Serial.println("[Incubator] ERROR: LTR390 not found");
    }
    else
    {
        _ltr.setMode(LTR390_MODE_UVS);
        _ltr.setGain(LTR390_GAIN_18);
        _ltr.setResolution(LTR390_RESOLUTION_20BIT);
        // 20-bit integration time ≈ 1000 ms; 500 ms margin = 1500 ms (datasheet requirement)
        delay(1500);
        Serial.println("[Incubator] LTR390 OK");
    }
}

void Incubator::read_All_Sensors()
{
    read_SHT35_Sensors();
    read_UV_Sensor();
    tick_CO2_State_Machine();
}

void Incubator::set_ITO_Power(uint8_t power)
{
    ledcWrite(ITO_PWM_CH, power);
}

void Incubator::update_Heater_PWM()
{
    float error = targetTemperature - temp2;
    int power = constrain((int)(error * ITO_KP), 0, 255);
    set_ITO_Power((uint8_t)power);
}