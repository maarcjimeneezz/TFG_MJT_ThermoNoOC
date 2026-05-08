#include "Microfluidics.h"

Microfluidics::Microfluidics() {}

void Microfluidics::selectBus(uint8_t channel)
{
    // Uses MUX_ADDR_MICROFLUIDICS (0x71) from Pinout.h
    Wire.beginTransmission(MUX_ADDR_MICROFLUIDICS);
    Wire.write(1 << channel);
    Wire.endTransmission();
}

void Microfluidics::setPumpPage(uint8_t page)
{
    // Write to the page register at address 0xFF
    Wire.beginTransmission(pumpAddr);
    Wire.write(0xFF);
    Wire.write(page);
    Wire.endTransmission();
}

void Microfluidics::begin()
{
    // --- 1. Initialize all 4 Pumps (Channels 2, 3, 4, 5) ---
    uint8_t pumpChannels[4] = {MUX_CH_PUMP_1, MUX_CH_PUMP_2, MUX_CH_PUMP_3, MUX_CH_PUMP_4};

    for (int i = 0; i < 4; i++)
    {
        selectBus(pumpChannels[i]);

        // Initialize mp-Lowdriver startup sequence
        setPumpPage(0x00);
        Wire.beginTransmission(pumpAddr);
        Wire.write(0x01);
        Wire.write(0x02); // Set Gain
        Wire.write(0x00);
        Wire.write(0x01); // Wake & Play Waveform #1
        Wire.endTransmission();

        // Memory Header setup for Waveform Synthesis
        setPumpPage(0x01);
        Wire.beginTransmission(pumpAddr);
        Wire.write(0x00);
        Wire.write(0x05); // Header
        Wire.write(0x80);
        Wire.write(0x06); // Start addr
        Wire.write(0x00);
        Wire.write(0x09); // Stop addr
        Wire.write(0x00); // Infinite repeat
        Wire.endTransmission();
    }

    // --- 2. Initialize both Flow Sensors (Channels 6, 7) ---
    uint8_t flowChannels[2] = {MUX_CH_FLOW_1, MUX_CH_FLOW_2};
    for (int i = 0; i < 2; i++)
    {
        selectBus(flowChannels[i]);
        Wire.beginTransmission(flowAddr);
        Wire.write(0x36);
        Wire.write(0x08); // Start continuous measurement
        Wire.endTransmission();
    }
}

float Microfluidics::getFlowRate(int sensorNum)
{
    uint8_t channel = (sensorNum == 2) ? MUX_CH_FLOW_2 : MUX_CH_FLOW_1;
    selectBus(channel);

    Wire.requestFrom(flowAddr, (uint8_t)3); // Read 2 bytes + CRC
    if (Wire.available() >= 2)
    {
        int16_t rawFlow = (Wire.read() << 8) | Wire.read();
        Wire.read();                    // Discard CRC
        return (float)rawFlow / 500.0f; // SLF3S-0600F Scale factor
    }
    return 0.0f;
}

void Microfluidics::setPumpVoltage(int pumpNum, uint8_t voltage)
{
    uint8_t channels[4] = {MUX_CH_PUMP_1, MUX_CH_PUMP_2, MUX_CH_PUMP_3, MUX_CH_PUMP_4};
    if (pumpNum < 1 || pumpNum > 4)
        return;

    selectBus(channels[pumpNum - 1]);

    setPumpPage(0x00);
    Wire.beginTransmission(pumpAddr);
    Wire.write(0x02);
    Wire.write(0x00); // Stop
    Wire.endTransmission();

    setPumpPage(0x01);
    Wire.beginTransmission(pumpAddr);
    Wire.write(0x06);
    Wire.write(voltage); // Update Amplitude
    Wire.endTransmission();

    delay(5);

    setPumpPage(0x00);
    Wire.beginTransmission(pumpAddr);
    Wire.write(0x02);
    Wire.write(0x01); // Resume
    Wire.endTransmission();
}

void Microfluidics::setPumpFrequency(int pumpNum, uint16_t frequency)
{
    uint8_t channels[4] = {MUX_CH_PUMP_1, MUX_CH_PUMP_2, MUX_CH_PUMP_3, MUX_CH_PUMP_4};
    if (pumpNum < 1 || pumpNum > 4)
        return;

    selectBus(channels[pumpNum - 1]);

    uint8_t freqByte = (uint8_t)(frequency / 7.8125f);
    if (freqByte == 0)
        freqByte = 1;

    setPumpPage(0x00);
    Wire.beginTransmission(pumpAddr);
    Wire.write(0x02);
    Wire.write(0x00); // Stop
    Wire.endTransmission();

    setPumpPage(0x01);
    Wire.beginTransmission(pumpAddr);
    Wire.write(0x07);
    Wire.write(freqByte); // Update Frequency
    Wire.endTransmission();

    delay(5);

    setPumpPage(0x00);
    Wire.beginTransmission(pumpAddr);
    Wire.write(0x02);
    Wire.write(0x01); // Resume
    Wire.endTransmission();
}