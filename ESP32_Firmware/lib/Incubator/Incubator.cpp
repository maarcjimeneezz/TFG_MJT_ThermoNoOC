#include "Incubator.h"

// Constructor: Initializing SHT objects using auto-detection
Incubator::Incubator() : sht1(SHTSensor::AUTO_DETECT), sht2(SHTSensor::AUTO_DETECT)
{
    temp1 = hum1 = temp2 = hum2 = uvIndex = 0.0f;
    co2Percent = 0.0f;
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
    Wire.begin(I2C_SDA, I2C_SCL);

    // --- Initialize Serial2 for CO2 Sensor (9600 baud) ---
    Serial2.begin(19200, SERIAL_8N1, PIN_CO2_RX, PIN_CO2_TX);

    // --- Initialize LED Array ---
    leds.begin();

    // --- Initialize Microfluidics (pumps and flow sensors) ---
    fluidics.begin();

    // --- Initialize ITO Glass PWM ---
    ledcSetup(itoCh, itoFreq, itoRes);
    ledcAttachPin(PIN_ITO_CONTROL, itoCh);
    setITOPower(0);

    // --- Initialize I2C Sensors through the Multiplexer ---
    // Sensor 1 (SHT35)
    Serial.println("Initializing SHT35 #1...");
    selectBus(MUX_CH_TEMP1);
    delay(50);
    if (!sht1.init())
    {
        Serial.println("ERROR: SHT35 #1 init failed - sensor not detected or I2C error");
    }
    else
    {
        Serial.println("SHT35 #1 initialized successfully");
    }

    // Sensor 2 (SHT35)
    Serial.println("Initializing SHT35 #2...");
    selectBus(MUX_CH_TEMP2);
    delay(50);
    if (!sht2.init())
    {
        Serial.println("ERROR: SHT35 #2 init failed - sensor not detected or I2C error");
    }
    else
    {
        Serial.println("SHT35 #2 initialized successfully");
    }

    // UV Sensor (LTR390)
    selectBus(MUX_CH_UV);
    if (!ltr.begin())
    {
        Serial.println("LTR390 init failed");
    }
    else
    {
        ltr.setMode(LTR390_MODE_UVS);
        ltr.setGain(LTR390_GAIN_18); // high sensitivity for low UV levels
        ltr.setResolution(LTR390_RESOLUTION_20BIT);
        // LTR390 needs time to acquire first measurement (integration time + stabilization)
        // With 20-bit resolution, measurement takes ~1000ms; add 500ms margin for stability
        delay(1500);
        Serial.println("LTR390 initialized (waiting for first data...)");
    }
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
    else
    {
        Serial.println("ERROR: SHT35 #1 read failed - check I2C connection");
        temp1 = 0.0f;
        hum1 = 0.0f;
    }

    // 2. Read SHT35 - Channel 2
    selectBus(MUX_CH_TEMP2);
    if (sht2.readSample())
    {
        temp2 = sht2.getTemperature();
        hum2 = sht2.getHumidity();
    }
    else
    {
        Serial.println("ERROR: SHT35 #2 read failed - check I2C connection");
        temp2 = 0.0f;
        hum2 = 0.0f;
    }

    // 3. Read UV Sensor
    selectBus(MUX_CH_UV);
    delay(50);
    if (ltr.newDataAvailable())
    {
        // Leer el valor en Raw Counts
        uint32_t rawUV = ltr.readUVS();

        // Convertir a UV Index
        // Fórmula: UVI = Raw Counts / Factor de Sensibilidad * WFAC
        // Para Gain=18x y Res=20-bit, el factor es 2300.0
        uvIndex = (float)rawUV / 2300.0f;

        // Convertir a Irradiancia (W/m²)
        uvIrradiance = uvIndex * 0.025f;
    }
    else
    {
        Serial.printf("WARNING: LTR390 no new data available (mode=%d gain=%d res=%d)\n",
                      ltr.getMode(), ltr.getGain(), ltr.getResolution());
        uvIndex = 0.0f;
        uvIrradiance = 0.0f;
    }

    // 4. Read CO2 Sensor via Serial2
    // Limpieza profunda del puerto antes de transmitir [cite: 34]
    Serial2.flush();
    while (Serial2.available() > 0)
        Serial2.read();

    // Comando exacto según página 8 del manual: FF FE 02 02 03 [cite: 150]
    uint8_t cmd[] = {0xFF, 0xFE, 0x02, 0x02, 0x03};
    Serial2.write(cmd, 5);

    // El manual dice que el ciclo DSP puede tardar segundos, pero la
    // respuesta UART debe ser rápida tras el comando [cite: 16, 19]
    unsigned long timeout = millis() + 500;
    while (Serial2.available() < 5 && millis() < timeout)
    {
        delay(10);
    }

    if (Serial2.available() >= 5)
    {
        uint8_t resp[5];
        Serial2.readBytes(resp, 5);

        // Validar cabecera de respuesta: FF FA (To Master) [cite: 78, 83]
        if (resp[0] == 0xFF && resp[1] == 0xFA)
        {
            // El manual indica que el PPM viene en los bytes 3 y 4 (MSB y LSB) [cite: 116, 150]
            uint16_t co2Raw = (resp[3] << 8) | resp[4];

            // IMPORTANTE: Algunos modelos devuelven el valor para multiplicar por 16 [cite: 116, 150]
            // Si ves un valor de ~25 en aire normal, cámbialo a (co2Raw * 16)
            co2Percent = (float)co2Raw / 10000.0f;
        }
        else
        {
            Serial.printf("Error de Sincronía: %02X %02X\n", resp[0], resp[1]);
        }
    }
    else
    {
        // El manual sugiere re-enviar si no hay respuesta [cite: 12, 34]
        Serial.println("CO2 Timeout: Sensor no respondió al comando 0x02 0x03");
    }
}

void Incubator::setITOPower(uint8_t power)
{
    // Apply PWM to the ITO control pin
    ledcWrite(itoCh, power);
}

void Incubator::updateActuators()
{
    // --- 1. Temperature Control (ITO Glass Heater) ---
    // Simple proportional control: increase power if temp is below target
    float avgTemp = (temp1 + temp2) / 2.0;
    float tempError = targetTemperature - avgTemp;

    // P-controller: Kp = 10, range 0-255
    int itopower = (int)(tempError * 10.0);
    itopower = constrain(itopower, 0, 255);
    setITOPower((uint8_t)itopower);

    // --- 2. UV LED Control ---
    // Update UV LEDs based on enabled groups and intensity
    for (int i = 0; i < 4; i++)
    {
        if (uvEnabled[i])
        {
            leds.setBrightness(i + 1, uvIntensity[i]);
        }
        else
        {
            leds.setBrightness(i + 1, 0);
        }
    }

    // --- 3. Pump Flow Control ---
    // Update pump configurations based on mode (continuous or pulsed)
    // Each circuit has a fluid pump and an air pump that work together
    for (int circuitIdx = 0; circuitIdx < 2; circuitIdx++)
    {
        int fluidPump = circuitIdx + 1; // Pumps 1 and 2 (fluid)
        int airPump = circuitIdx + 3;   // Pumps 3 and 4 (air for bubble removal)

        if (pumpConfig[circuitIdx].flow > 0)
        {
            // Convert flow rate (µL/min) to pump frequency (Hz)
            // Mapping: 0-2000 µL/min -> 0-800 Hz (freq = flow / 2.5)
            uint16_t freq = (uint16_t)(pumpConfig[circuitIdx].flow / 2.5);
            freq = constrain(freq, 8, 800); // Constrain to valid frequency range

            // Set both fluid and air pumps to the same frequency and voltage
            fluidics.setPumpFrequency(fluidPump, freq);
            fluidics.setPumpVoltage(fluidPump, 200); // Default amplitude (0-150 Vpp)
            fluidics.setPumpFrequency(airPump, freq);
            fluidics.setPumpVoltage(airPump, 200); // Same amplitude for air pump

            // Handle pulsed mode
            if (pumpConfig[circuitIdx].mode == "pulsed" && pumpConfig[circuitIdx].cycles > 0)
            {
                static unsigned long lastPulseTime[2] = {0, 0};
                static int pulseCount[2] = {0, 0};
                unsigned long currentTime = millis();

                float cyclePeriod = (pumpConfig[circuitIdx].feedingTime + pumpConfig[circuitIdx].pauseTime) * 1000; // Convert to ms
                float pulsePeriod = currentTime - lastPulseTime[circuitIdx];

                if (pulsePeriod >= cyclePeriod)
                {
                    pulseCount[circuitIdx]++;
                    lastPulseTime[circuitIdx] = currentTime;
                }

                // Stop both pumps in the circuit if cycles are complete
                if (pulseCount[circuitIdx] >= pumpConfig[circuitIdx].cycles)
                {
                    fluidics.setPumpVoltage(fluidPump, 0);
                    fluidics.setPumpVoltage(airPump, 0);
                    pulseCount[circuitIdx] = 0;
                }
            }
        }
        else
        {
            // Turn off both pumps in the circuit
            fluidics.setPumpVoltage(fluidPump, 0);
            fluidics.setPumpVoltage(airPump, 0);
        }
    }

    // Update flow sensor readings
    flow1 = fluidics.getFlowRate(1);
    flow2 = fluidics.getFlowRate(2);
}