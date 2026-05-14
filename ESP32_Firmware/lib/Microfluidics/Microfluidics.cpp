#include "Microfluidics.h"

const uint8_t Microfluidics::PUMP_MUX_CH[4] = {
    MUX_CH_PUMP_1, MUX_CH_PUMP_2, MUX_CH_PUMP_3, MUX_CH_PUMP_4};
const uint8_t Microfluidics::FLOW_MUX_CH[2] = {
    MUX_CH_FLOW_1, MUX_CH_FLOW_2};

Microfluidics::Microfluidics()
{
    for (int i = 0; i < NUM_CIRCUITS; i++)
    {
        _lastCycleTime[i] = 0;
        _cycleCount[i] = 0;
    }
    for (int i = 0; i < NUM_PUMPS; i++)
    {
        _curVoltage[i] = 0;
        _curFreqByte[i] = 0;
    }
}

// ---------------------------------------------------------------------------
// Low-level hardware helpers
// ---------------------------------------------------------------------------

void Microfluidics::select_Mux_Bus(uint8_t muxChannel)
{
    Wire.beginTransmission(MUX_ADDR_MICROFLUIDICS);
    Wire.write(1 << muxChannel);
    Wire.endTransmission();
}

void Microfluidics::set_Pump_Register_Page(uint8_t page)
{
    Wire.beginTransmission(PUMP_I2C_ADDR);
    Wire.write(0xFF);
    Wire.write(page);
    Wire.endTransmission();
}

void Microfluidics::send_Pump_Stop(int pumpIdx)
{
    select_Mux_Bus(PUMP_MUX_CH[pumpIdx]);
    set_Pump_Register_Page(0x00);
    Wire.beginTransmission(PUMP_I2C_ADDR);
    Wire.write(0x02);
    Wire.write(0x00); // STOP
    Wire.endTransmission();
}

void Microfluidics::send_Pump_Resume(int pumpIdx)
{
    select_Mux_Bus(PUMP_MUX_CH[pumpIdx]);
    set_Pump_Register_Page(0x00);
    Wire.beginTransmission(PUMP_I2C_ADDR);
    Wire.write(0x02);
    Wire.write(0x01); // RESUME
    Wire.endTransmission();
}

void Microfluidics::apply_Pending_Update(int pumpIdx)
{
    select_Mux_Bus(PUMP_MUX_CH[pumpIdx]);
    set_Pump_Register_Page(0x01);

    if (_pending[pumpIdx].hasVoltage)
    {
        Wire.beginTransmission(PUMP_I2C_ADDR);
        Wire.write(0x06);
        Wire.write(_pending[pumpIdx].voltage);
        Wire.endTransmission();
        _curVoltage[pumpIdx] = _pending[pumpIdx].voltage;
        _pending[pumpIdx].hasVoltage = false;
    }

    if (_pending[pumpIdx].hasFreq)
    {
        Wire.beginTransmission(PUMP_I2C_ADDR);
        Wire.write(0x07);
        Wire.write(_pending[pumpIdx].freqByte);
        Wire.endTransmission();
        _curFreqByte[pumpIdx] = _pending[pumpIdx].freqByte;
        _pending[pumpIdx].hasFreq = false;
    }

    send_Pump_Resume(pumpIdx);
    _pending[pumpIdx].active = false;
}

void Microfluidics::init_Single_Pump(int pumpIdx)
{
    select_Mux_Bus(PUMP_MUX_CH[pumpIdx]);

    set_Pump_Register_Page(0x00);
    Wire.beginTransmission(PUMP_I2C_ADDR);
    Wire.write(0x01);
    Wire.write(0x02); // Set Gain
    Wire.write(0x00);
    Wire.write(0x01); // Wake & play Waveform #1
    Wire.endTransmission();

    set_Pump_Register_Page(0x01);
    Wire.beginTransmission(PUMP_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0x05); // Header
    Wire.write(0x80);
    Wire.write(0x06); // Start address
    Wire.write(0x00);
    Wire.write(0x09); // Stop address
    Wire.write(0x00); // Infinite repeat
    Wire.endTransmission();
}

void Microfluidics::init_Single_Flow_Sensor(int sensorIdx)
{
    select_Mux_Bus(FLOW_MUX_CH[sensorIdx]);
    Wire.beginTransmission(FLOW_I2C_ADDR);
    Wire.write(0x36);
    Wire.write(0x08); // Start continuous measurement
    Wire.endTransmission();
}

// ---------------------------------------------------------------------------
// Pending-update queue — non-blocking STOP → settle → write → RESUME
// ---------------------------------------------------------------------------

void Microfluidics::queue_Pump_Voltage(int pumpIdx, uint8_t voltage)
{
    if (_pending[pumpIdx].active)
    {
        // STOP already in flight; just update the queued value
        _pending[pumpIdx].voltage = voltage;
        _pending[pumpIdx].hasVoltage = true;
        return;
    }
    if (_curVoltage[pumpIdx] == voltage)
        return; // No change needed

    _pending[pumpIdx].voltage = voltage;
    _pending[pumpIdx].hasVoltage = true;
    _pending[pumpIdx].active = true;
    _pending[pumpIdx].stopSentAt = millis();
    send_Pump_Stop(pumpIdx);
}

void Microfluidics::queue_Pump_Freq_Byte(int pumpIdx, uint8_t freqByte)
{
    if (_pending[pumpIdx].active)
    {
        _pending[pumpIdx].freqByte = freqByte;
        _pending[pumpIdx].hasFreq = true;
        return;
    }
    if (_curFreqByte[pumpIdx] == freqByte)
        return;

    _pending[pumpIdx].freqByte = freqByte;
    _pending[pumpIdx].hasFreq = true;
    _pending[pumpIdx].active = true;
    _pending[pumpIdx].stopSentAt = millis();
    send_Pump_Stop(pumpIdx);
}

void Microfluidics::process_Pending_Updates()
{
    for (int i = 0; i < NUM_PUMPS; i++)
    {
        if (_pending[i].active && (millis() - _pending[i].stopSentAt >= PUMP_SETTLE_MS))
            apply_Pending_Update(i);
    }
}

// ---------------------------------------------------------------------------
// Circuit logic helpers
// ---------------------------------------------------------------------------

uint8_t Microfluidics::flow_Rate_To_Freq_Byte(float flowRate_uLmin) const
{
    uint16_t hz = (uint16_t)constrain((int)(flowRate_uLmin / FLOW_TO_HZ),
                                      (int)FREQ_MIN_HZ, (int)FREQ_MAX_HZ);
    uint8_t freqByte = (uint8_t)(hz / HZ_PER_BIT);
    return (freqByte == 0) ? 1 : freqByte;
}

void Microfluidics::stop_Circuit_Pumps(int circuitIdx)
{
    // circuitIdx 0 → pumps 0,2 (fluid=0, air=2)
    // circuitIdx 1 → pumps 1,3 (fluid=1, air=3)
    queue_Pump_Voltage(circuitIdx, 0);
    queue_Pump_Voltage(circuitIdx + 2, 0);
}

void Microfluidics::run_Circuit_Pumps(int circuitIdx)
{
    uint8_t freqByte = flow_Rate_To_Freq_Byte(_config[circuitIdx].flowRate_uLmin);
    int fluidPump = circuitIdx;
    int airPump = circuitIdx + 2;

    queue_Pump_Freq_Byte(fluidPump, freqByte);
    queue_Pump_Voltage(fluidPump, 200);
    queue_Pump_Freq_Byte(airPump, freqByte);
    queue_Pump_Voltage(airPump, 200);
}

void Microfluidics::apply_Circuit_Continuous(int circuitIdx)
{
    run_Circuit_Pumps(circuitIdx);
}

void Microfluidics::apply_Circuit_Pulsed(int circuitIdx)
{
    const PumpConfig &cfg = _config[circuitIdx];

    // Infinite when cycles == 0; otherwise stop once all cycles complete
    if (cfg.cycles > 0 && _cycleCount[circuitIdx] >= cfg.cycles)
    {
        stop_Circuit_Pumps(circuitIdx);
        return;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - _lastCycleTime[circuitIdx];
    unsigned long feedMs = (unsigned long)(cfg.feedTime_s * 1000.0f);
    unsigned long cycleMs = feedMs + (unsigned long)(cfg.pauseTime_s * 1000.0f);

    if (elapsed >= cycleMs)
    {
        _cycleCount[circuitIdx]++;
        _lastCycleTime[circuitIdx] = now;
        elapsed = 0;
    }

    if (elapsed < feedMs)
        run_Circuit_Pumps(circuitIdx); // Feed phase
    else
        stop_Circuit_Pumps(circuitIdx); // Pause phase
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Microfluidics::begin()
{
    Wire.begin(I2C_SDA, I2C_SCL);
    for (int i = 0; i < NUM_PUMPS; i++)
        init_Single_Pump(i);
    for (int i = 0; i < NUM_SENSORS; i++)
        init_Single_Flow_Sensor(i);
}

void Microfluidics::set_Circuit_Config(int circuit, const PumpConfig &config)
{
    if (circuit < 1 || circuit > NUM_CIRCUITS)
        return;
    int idx = circuit - 1;
    _config[idx] = config;
    _cycleCount[idx] = 0;
    _lastCycleTime[idx] = millis();
}

void Microfluidics::update_Pumps()
{
    process_Pending_Updates();

    for (int i = 0; i < NUM_CIRCUITS; i++)
    {
        if (_config[i].flowRate_uLmin <= 0.0f)
            stop_Circuit_Pumps(i);
        else if (_config[i].pulsed)
            apply_Circuit_Pulsed(i);
        else
            apply_Circuit_Continuous(i);
    }
}

float Microfluidics::read_Flow_Rate(int sensorNum)
{
    if (sensorNum < 1 || sensorNum > NUM_SENSORS)
        return 0.0f;
    select_Mux_Bus(FLOW_MUX_CH[sensorNum - 1]);
    Wire.requestFrom(FLOW_I2C_ADDR, (uint8_t)3);
    if (Wire.available() >= 2)
    {
        int16_t raw = (Wire.read() << 8) | Wire.read();
        Wire.read();                            // discard CRC
        return ((float)raw / 500.0f) * 1000.0f; // ml/min → µL/min
    }
    return 0.0f;
}

void Microfluidics::set_Pump_Voltage(int pumpNum, uint8_t voltage)
{
    if (pumpNum < 1 || pumpNum > NUM_PUMPS)
        return;
    queue_Pump_Voltage(pumpNum - 1, voltage);
}

void Microfluidics::set_Pump_Frequency(int pumpNum, uint16_t frequency_Hz)
{
    if (pumpNum < 1 || pumpNum > NUM_PUMPS)
        return;
    uint8_t freqByte = (uint8_t)(frequency_Hz / HZ_PER_BIT);
    queue_Pump_Freq_Byte(pumpNum - 1, (freqByte == 0) ? 1 : freqByte);
}