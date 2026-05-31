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
        _lastFlowReading[i] = 0.0f;
        _pid[i] = PidState{};
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
    // Manufacturer calibration table at 100 Vpp
    static const float FLOW_LUT[] = {350.f, 650.f, 900.f, 1350.f, 1550.f, 1950.f, 2250.f};
    static const float HZ_LUT[] = {10.f, 20.f, 30.f, 40.f, 50.f, 75.f, 100.f};
    static const int N = 7;

    float hz;
    if (flowRate_uLmin <= FLOW_LUT[0])
    {
        hz = HZ_LUT[0];
    }
    else if (flowRate_uLmin >= FLOW_LUT[N - 1])
    {
        hz = HZ_LUT[N - 1];
    }
    else
    {
        hz = HZ_LUT[N - 1];
        for (int i = 0; i < N - 1; i++)
        {
            if (flowRate_uLmin <= FLOW_LUT[i + 1])
            {
                float t = (flowRate_uLmin - FLOW_LUT[i]) / (FLOW_LUT[i + 1] - FLOW_LUT[i]);
                hz = HZ_LUT[i] + t * (HZ_LUT[i + 1] - HZ_LUT[i]);
                break;
            }
        }
    }

    uint8_t freqByte = (uint8_t)(hz / HZ_PER_BIT);
    return (freqByte == 0) ? 1 : freqByte;
}

// Pump index helpers — circuit layout:
//   Circuit 0 (UI circuit 1): pump 0 = fluid (pump 1), pump 2 = bubble (pump 3)
//   Circuit 1 (UI circuit 2): pump 1 = fluid (pump 2), pump 3 = bubble (pump 4)
static inline int fluid_Pump(int circuitIdx) { return circuitIdx; }
static inline int bubble_Pump(int circuitIdx) { return circuitIdx + 2; }

void Microfluidics::stop_Circuit_Pumps(int circuitIdx)
{
    queue_Pump_Voltage(fluid_Pump(circuitIdx), 0);
    queue_Pump_Voltage(bubble_Pump(circuitIdx), 0);
}

// ---------------------------------------------------------------------------
// PID — feedforward + PI correction; runs at PID_INTERVAL_MS cadence.
// Reads the flow sensor for this circuit, updates _lastFlowReading, and
// writes a new outputFreqByte into _pid[circuitIdx].
// ---------------------------------------------------------------------------
void Microfluidics::update_Fluid_Pump_Pid(int circuitIdx)
{
    unsigned long now = millis();
    PidState &pid = _pid[circuitIdx];

    if (pid.lastMs != 0 && (now - pid.lastMs) < PID_INTERVAL_MS)
        return; // Not time yet — keep using previous output

    float dt = (pid.lastMs == 0) ? (PID_INTERVAL_MS / 1000.0f)
                                 : (now - pid.lastMs) / 1000.0f;
    pid.lastMs = now;

    // Sample flow sensor (circuit 0 → sensor 1, circuit 1 → sensor 2)
    float measured = read_Flow_Rate(circuitIdx + 1);
    _lastFlowReading[circuitIdx] = measured;

    float setpoint = _config[circuitIdx].flowRate_uLmin;
    float error = setpoint - measured;

    // Feedforward gives a calibrated initial operating point; PI trims the error
    float ff = (float)flow_Rate_To_Freq_Byte(setpoint);

    float newIntegral = pid.integral + error * dt;
    float rawOutput = ff + PID_KP * error + PID_KI * newIntegral;

    float maxByte = FREQ_MAX_HZ / HZ_PER_BIT;
    float clamped = constrain(rawOutput, 1.0f, maxByte);

    // Anti-windup: freeze integral when output saturates against the error direction
    if (clamped == rawOutput)
        pid.integral = newIntegral;

    pid.prevError = error;
    pid.outputFreqByte = clamped;
}

void Microfluidics::apply_Circuit_Continuous(int circuitIdx)
{
    update_Fluid_Pump_Pid(circuitIdx);

    uint8_t fluidFreq = (uint8_t)_pid[circuitIdx].outputFreqByte;

    // Fluid pump: PID-controlled frequency + fixed voltage
    queue_Pump_Freq_Byte(fluid_Pump(circuitIdx), fluidFreq);
    queue_Pump_Voltage(fluid_Pump(circuitIdx), FLUID_VOLTAGE);

    // Bubble-removal pump: fixed frequency/voltage whenever fluid pump is running
    queue_Pump_Freq_Byte(bubble_Pump(circuitIdx), BUBBLE_FREQ_BYTE);
    queue_Pump_Voltage(bubble_Pump(circuitIdx), BUBBLE_VOLTAGE);
}

void Microfluidics::apply_Circuit_Pulsed(int circuitIdx)
{
    const PumpConfig &cfg = _config[circuitIdx];

    // Infinite when cycles == 0; stop once all cycles complete
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
        apply_Circuit_Continuous(circuitIdx); // Feed phase — PID active
    else
        stop_Circuit_Pumps(circuitIdx); // Pause phase — both pumps off
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
    delay(60); // SLF3S-0600F: 60 ms warm-up after start continuous measurement
}

void Microfluidics::set_Circuit_Config(int circuit, const PumpConfig &config)
{
    if (circuit < 1 || circuit > NUM_CIRCUITS)
        return;
    int idx = circuit - 1;
    _config[idx] = config;
    _cycleCount[idx] = 0;
    _lastCycleTime[idx] = millis();
    _pid[idx] = PidState{}; // Reset PID so new setpoint starts fresh
}

void Microfluidics::stop_All()
{
    for (int i = 0; i < NUM_PUMPS; i++)
        queue_Pump_Voltage(i, 0);
    for (int i = 0; i < NUM_CIRCUITS; i++)
        _pid[i] = PidState{}; // Reset PID so next run starts clean
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
        return (float)raw / 10.0f; // SLF3S-0600F: scale factor 10 LSB/(µL/min)
    }
    return 0.0f;
}

float Microfluidics::get_Last_Flow_Reading(int circuitNum) const
{
    if (circuitNum < 1 || circuitNum > NUM_CIRCUITS)
        return 0.0f;
    return _lastFlowReading[circuitNum - 1];
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