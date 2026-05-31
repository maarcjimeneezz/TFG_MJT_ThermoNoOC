/**
 * @file test_pumps_logic.cpp
 * @brief Circuit-level pump validation via Serial monitor.
 *
 * Direct pump control — bypasses PID and circuit logic entirely.
 * Calls process_Pending_Updates() instead of update_Pumps() so the
 * circuit state machine never overrides the commanded values.
 *
 * Circuit layout:
 *   Circuit 1 — pump 1 (fluid) + pump 3 (air bubble removal)
 *   Circuit 2 — pump 2 (fluid) + pump 4 (air bubble removal)
 *
 * Commands (newline-terminated):
 *   c1 <uL/min>                              Continuous, circuit 1  (0 = off)
 *   c2 <uL/min>                              Continuous, circuit 2  (0 = off)
 *   c1 p <uL/min> <feed_s> <pause_s> [N]    Pulsed, circuit 1  (N cycles, 0 = infinite)
 *   c2 p <uL/min> <feed_s> <pause_s> [N]    Pulsed, circuit 2
 *   off                                      Stop all pumps
 *   ?                                        Print current circuit configs
 *
 * Examples:
 *   c1 500               -> circuit 1 continuous at 500 uL/min
 *   c2 p 800 2.0 3.0 5   -> circuit 2: 800 uL/min, 2s on / 3s off, 5 cycles
 *   off                  -> stop both circuits
 */

#include <Arduino.h>
#include <esp_log.h>
#include "Microfluidics.h"

// Mirror of private constants from Microfluidics.cpp
static const uint8_t FLUID_V_BYTE = 168;  // ~100 Vpp register byte
static const uint8_t BUBBLE_V_BYTE = 168; // ~100 Vpp register byte
static const uint16_t BUBBLE_HZ = 156;    // BUBBLE_FREQ_BYTE(20) * 7.8125

// Circuit-to-pump mapping
static const int FLUID_PUMP[2] = {1, 2};  // pump 1 = C1 fluid,  pump 2 = C2 fluid
static const int BUBBLE_PUMP[2] = {3, 4}; // pump 3 = C1 bubble, pump 4 = C2 bubble

static Microfluidics fluidics;
static String _inputBuf = "";

struct CircuitState
{
    float flowRate = 0.0f;
    bool pulsed = false;
    float feedTime = 0.0f;
    float pauseTime = 0.0f;
    int cycles = 0;
    int cyclesDone = 0;
    bool inFeed = false;
    unsigned long phaseStart = 0;
};
static CircuitState _state[2];

static uint16_t flow_to_hz(float uLmin)
{
    // Manufacturer calibration table at 100 Vpp
    static const float FLOW_LUT[] = {350.f, 650.f, 900.f, 1350.f, 1550.f, 1950.f, 2250.f};
    static const float HZ_LUT[] = {10.f, 20.f, 30.f, 40.f, 50.f, 75.f, 100.f};
    static const int N = 7;

    if (uLmin <= FLOW_LUT[0])
        return (uint16_t)HZ_LUT[0];
    if (uLmin >= FLOW_LUT[N - 1])
        return (uint16_t)HZ_LUT[N - 1];

    for (int i = 0; i < N - 1; i++)
    {
        if (uLmin <= FLOW_LUT[i + 1])
        {
            float t = (uLmin - FLOW_LUT[i]) / (FLOW_LUT[i + 1] - FLOW_LUT[i]);
            return (uint16_t)(HZ_LUT[i] + t * (HZ_LUT[i + 1] - HZ_LUT[i]) + 0.5f);
        }
    }
    return (uint16_t)HZ_LUT[N - 1];
}

static void start_Pumps(int idx)
{
    fluidics.set_Pump_Frequency(FLUID_PUMP[idx], flow_to_hz(_state[idx].flowRate));
    fluidics.set_Pump_Voltage(FLUID_PUMP[idx], FLUID_V_BYTE);
    // Air pumps (3 & 4) intentionally off during fluid-only test
}

static void stop_Pumps(int idx)
{
    fluidics.set_Pump_Voltage(FLUID_PUMP[idx], 0);
    fluidics.set_Pump_Voltage(BUBBLE_PUMP[idx], 0);
}

static void print_Config(int idx)
{
    Serial.print("  Circuit ");
    Serial.print(idx + 1);
    Serial.print(": ");
    if (_state[idx].flowRate <= 0.0f)
    {
        Serial.println("OFF");
        return;
    }
    Serial.print(_state[idx].flowRate, 1);
    Serial.print(" uL/min (");
    Serial.print(flow_to_hz(_state[idx].flowRate));
    Serial.print(" Hz)");
    if (_state[idx].pulsed)
    {
        Serial.print(" | pulsed feed=");
        Serial.print(_state[idx].feedTime, 1);
        Serial.print("s pause=");
        Serial.print(_state[idx].pauseTime, 1);
        Serial.print("s cycles=");
        if (_state[idx].cycles == 0)
            Serial.print("inf");
        else
            Serial.print(_state[idx].cycles);
        Serial.print(" done=");
        Serial.print(_state[idx].cyclesDone);
    }
    else
    {
        Serial.print(" | continuous");
    }
    Serial.println();
}

static void handle_Command(const String &cmd)
{
    if (cmd == "off" || cmd == "OFF")
    {
        for (int i = 0; i < 2; i++)
        {
            _state[i] = CircuitState{};
            stop_Pumps(i);
        }
        fluidics.stop_All();
        Serial.println("All circuits stopped.");
        return;
    }

    if (cmd == "?")
    {
        Serial.println("--- Status ---");
        print_Config(0);
        print_Config(1);
        Serial.println("---");
        return;
    }

    if (cmd.length() < 2 || cmd[0] != 'c' || (cmd[1] != '1' && cmd[1] != '2'))
    {
        Serial.println("ERR: unknown command. Type ? for help.");
        return;
    }

    int idx = cmd[1] - '1';
    String rest = cmd.substring(2);
    rest.trim();

    if (rest.length() == 0)
    {
        Serial.println("ERR: missing arguments.");
        return;
    }

    // --- Pulsed mode: p <flowrate> <feed_s> <pause_s> [cycles] ---
    if (rest[0] == 'p' || rest[0] == 'P')
    {
        rest = rest.substring(1);
        rest.trim();

        int s1 = rest.indexOf(' ');
        if (s1 < 0)
        {
            Serial.println("ERR: c<N> p <uL/min> <feed_s> <pause_s> [cycles]");
            return;
        }
        float rate = rest.substring(0, s1).toFloat();
        rest = rest.substring(s1 + 1);
        rest.trim();

        int s2 = rest.indexOf(' ');
        if (s2 < 0)
        {
            Serial.println("ERR: c<N> p <uL/min> <feed_s> <pause_s> [cycles]");
            return;
        }
        float feed = rest.substring(0, s2).toFloat();
        rest = rest.substring(s2 + 1);
        rest.trim();

        int s3 = rest.indexOf(' ');
        float pause;
        int cycles = 0;
        if (s3 < 0)
        {
            pause = rest.toFloat();
        }
        else
        {
            pause = rest.substring(0, s3).toFloat();
            cycles = rest.substring(s3 + 1).toInt();
        }

        if (rate <= 0.0f || rate > 2000.0f)
        {
            Serial.println("ERR: flowrate 1-2000 uL/min");
            return;
        }
        if (feed <= 0.0f || pause < 0.0f)
        {
            Serial.println("ERR: feed_s > 0, pause_s >= 0");
            return;
        }

        _state[idx] = CircuitState{};
        _state[idx].flowRate = rate;
        _state[idx].pulsed = true;
        _state[idx].feedTime = feed;
        _state[idx].pauseTime = pause;
        _state[idx].cycles = cycles;
        _state[idx].inFeed = true;
        _state[idx].phaseStart = millis();
        start_Pumps(idx);

        Serial.print("Circuit ");
        Serial.print(idx + 1);
        Serial.print(" pulsed ");
        Serial.print(rate, 1);
        Serial.print(" uL/min, feed=");
        Serial.print(feed, 1);
        Serial.print("s pause=");
        Serial.print(pause, 1);
        Serial.print("s cycles=");
        if (cycles == 0)
            Serial.println("inf");
        else
            Serial.println(cycles);
        return;
    }

    // --- Continuous mode: c<N> <flowrate> ---
    float rate = rest.toFloat();
    if (rate < 0.0f || rate > 2000.0f)
    {
        Serial.println("ERR: flowrate 0-2000 uL/min (0=off)");
        return;
    }

    _state[idx] = CircuitState{};
    _state[idx].flowRate = rate;

    if (rate <= 0.0f)
    {
        stop_Pumps(idx);
        Serial.print("Circuit ");
        Serial.print(idx + 1);
        Serial.println(": stopped.");
    }
    else
    {
        start_Pumps(idx);
        Serial.print("Circuit ");
        Serial.print(idx + 1);
        Serial.print(": ");
        Serial.print(rate, 1);
        Serial.print(" uL/min -> ");
        Serial.print(flow_to_hz(rate));
        Serial.println(" Hz");
    }
}

void setup_pumps_logic()
{
    Serial.begin(115200);
    esp_log_level_set("i2c.master", ESP_LOG_NONE); // suppress timeout errors when sensors absent
    fluidics.begin();
    fluidics.stop_All();

    Serial.println();
    Serial.println("=== CIRCUIT PUMP TEST (direct control, no PID) ===");
    Serial.println("c1 <uL/min>                            continuous circuit 1");
    Serial.println("c2 <uL/min>                            continuous circuit 2");
    Serial.println("c1 p <uL/min> <feed_s> <pause_s> [N]  pulsed circuit 1");
    Serial.println("c2 p <uL/min> <feed_s> <pause_s> [N]  pulsed circuit 2");
    Serial.println("off                                    stop all");
    Serial.println("?                                      status");
    Serial.println();
    Serial.print("> ");
}

void loop_pumps_logic()
{
    // Drive non-blocking STOP->settle->write->RESUME without circuit logic
    fluidics.process_Pending_Updates();

    // Print flow sensor readings every second
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 1000)
    {
        lastPrint = millis();
        float f1 = fluidics.read_Flow_Rate(1);
        float f2 = fluidics.read_Flow_Rate(2);
        Serial.print("[Flow] C1: ");
        Serial.print(f1, 1);
        Serial.print(" uL/min | C2: ");
        Serial.print(f2, 1);
        Serial.println(" uL/min");
        Serial.print("> ");
    }

    // Pulsed-mode state machines
    for (int i = 0; i < 2; i++)
    {
        CircuitState &s = _state[i];
        if (!s.pulsed || s.flowRate <= 0.0f)
            continue;
        if (s.cycles > 0 && s.cyclesDone >= s.cycles)
            continue;

        unsigned long elapsed = millis() - s.phaseStart;

        if (s.inFeed && elapsed >= (unsigned long)(s.feedTime * 1000.0f))
        {
            stop_Pumps(i);
            s.inFeed = false;
            s.phaseStart = millis();
        }
        else if (!s.inFeed && elapsed >= (unsigned long)(s.pauseTime * 1000.0f))
        {
            s.cyclesDone++;
            if (s.cycles > 0 && s.cyclesDone >= s.cycles)
            {
                Serial.print("Circuit ");
                Serial.print(i + 1);
                Serial.println(": all cycles done.");
                s.flowRate = 0.0f;
            }
            else
            {
                start_Pumps(i);
                s.inFeed = true;
                s.phaseStart = millis();
            }
        }
    }

    // Non-blocking serial line reader
    while (Serial.available())
    {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r')
        {
            _inputBuf.trim();
            if (_inputBuf.length() > 0)
            {
                handle_Command(_inputBuf);
                _inputBuf = "";
                Serial.print("> ");
            }
        }
        else
        {
            _inputBuf += c;
        }
    }
}
