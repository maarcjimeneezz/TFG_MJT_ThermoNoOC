/**
 * @file main.cpp
 * @brief ThermoNoOC — top-level orchestrator.
 *
 * Each module owns exactly one domain. This file is the only place where
 * modules are instantiated and their data is wired together. No domain logic
 * lives here — only setup/loop coordination and WebSocket command dispatch.
 */

#include <Arduino.h>
#include "WiFiManager.h"
#include "Control.h"
#include "Incubator.h"
#include "LED_Array.h"
#include "Microfluidics.h"

// ---------------------------------------------------------------------------
// Network configuration
// ---------------------------------------------------------------------------
static const char *WIFI_SSID = "ThermoNoOC";
static const char *WIFI_PASSWORD = "thermonooc";
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GW(192, 168, 4, 1);
static const IPAddress AP_SN(255, 255, 255, 0);
static const uint16_t WS_PORT = 5000;

// ---------------------------------------------------------------------------
// Module instances — one per domain, no cross-ownership
// ---------------------------------------------------------------------------
WiFiManager wifi(WIFI_SSID, WIFI_PASSWORD, WS_PORT);
Control control;
Incubator incubator;
LED_Array leds;
Microfluidics fluidics;

// ---------------------------------------------------------------------------
// Safety interlock — all sensor/actuator activity is gated behind this flag.
// It is set to true only when the UI confirms the incubator lid is closed.
// ---------------------------------------------------------------------------
static bool is_Incubator_Closed = false;

// ---------------------------------------------------------------------------
// Telemetry timing — single 1 s broadcast containing all sensor data
// ---------------------------------------------------------------------------
static unsigned long lastTelemetryMs = 0;
static const unsigned long TELEMETRY_INTERVAL_MS = 1000;

// ---------------------------------------------------------------------------
// WebSocket command handler (registered in setup, executed by WiFiManager::loop)
// ---------------------------------------------------------------------------
void on_WebSocket_Event(uint8_t clientNum, WStype_t type, uint8_t *payload, size_t length)
{
    if (type != WStype_TEXT || length == 0)
        return;

    String msg = String((char *)payload);

    // --- Temperature setpoint ---
    // Expected format: "SET_TEMP:37.5"
    if (msg.startsWith("SET_TEMP:"))
        incubator.targetTemperature = msg.substring(9).toFloat();

    // --- LED group control ---
    // Expected format: "SET_LED:1:1:75"  (group, enabled 0/1, intensity 0-100)
    else if (msg.startsWith("SET_LED:"))
    {
        int g1 = msg.indexOf(':');
        int g2 = msg.indexOf(':', g1 + 1);
        int g3 = msg.indexOf(':', g2 + 1);
        int group = msg.substring(g1 + 1, g2).toInt();
        bool enabled = msg.substring(g2 + 1, g3).toInt() != 0;
        uint8_t inten = (uint8_t)msg.substring(g3 + 1).toInt();
        leds.set_Group_Enabled(group, enabled);
        leds.set_Group_Intensity(group, inten);
    }

    // --- Incubator safety interlock ---
    // Expected format: "SET_INCUBATOR:1" (closed) / "SET_INCUBATOR:0" (opened)
    else if (msg.startsWith("SET_INCUBATOR:"))
        is_Incubator_Closed = msg.substring(14).toInt() != 0;

    // --- Pump circuit configuration ---
    // Expected format: "SET_PUMP:1:500.0:0:0:0:0"
    //   fields: circuit, flowRate_uLmin, pulsed, feedTime_s, pauseTime_s, cycles
    else if (msg.startsWith("SET_PUMP:"))
    {
        int f[6]; // field start positions
        f[0] = msg.indexOf(':');
        for (int i = 1; i < 6; i++)
            f[i] = msg.indexOf(':', f[i - 1] + 1);

        Microfluidics::PumpConfig cfg;
        int circuit = msg.substring(f[0] + 1, f[1]).toInt();
        cfg.flowRate_uLmin = msg.substring(f[1] + 1, f[2]).toFloat();
        cfg.pulsed = msg.substring(f[2] + 1, f[3]).toInt() != 0;
        cfg.feedTime_s = msg.substring(f[3] + 1, f[4]).toFloat();
        cfg.pauseTime_s = msg.substring(f[4] + 1, f[5]).toFloat();
        cfg.cycles = msg.substring(f[5] + 1).toInt();
        fluidics.set_Circuit_Config(circuit, cfg);
    }
}

// ---------------------------------------------------------------------------
// Telemetry JSON builder — all sensor data in one 1 s broadcast
// ---------------------------------------------------------------------------
String build_Telemetry_JSON()
{
    String json = "{";
    json += "\"temp1\":" + String(incubator.temp1, 2) + ",";
    json += "\"hum1\":" + String(incubator.hum1, 1) + ",";
    json += "\"temp2\":" + String(incubator.temp2, 2) + ",";
    json += "\"hum2\":" + String(incubator.hum2, 1) + ",";
    json += "\"uvIndex\":" + String(incubator.uvIndex, 3) + ",";
    json += "\"uvW\":" + String(incubator.uvIrradiance, 4) + ",";
    json += "\"co2\":" + String(incubator.co2Percent, 4) + ",";
    json += "\"flow1\":" + String(fluidics.read_Flow_Rate(1), 1) + ",";
    json += "\"flow2\":" + String(fluidics.read_Flow_Rate(2), 1);
    json += "}";
    return json;
}

// ---------------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    wifi.begin(AP_IP, AP_GW, AP_SN);
    wifi.server().onEvent(on_WebSocket_Event);

    control.begin();
    incubator.begin();
    leds.begin();
    fluidics.begin();
}

void loop()
{
    // 1. WiFi always runs — needed to receive SET_INCUBATOR and other commands
    wifi.loop();

    // 2. Safety gate: heater, LEDs, pumps and sensors are all inhibited while
    //    the incubator lid is open to protect both the operator and the hardware.
    if (is_Incubator_Closed)
    {
        incubator.read_All_Sensors();
        incubator.update_Heater_PWM();
        control.update_Fan_Speed(control.read_PCB_Temperature());
        leds.update_All_Groups();
        fluidics.update_Pumps();
    }

    // 3. Telemetry always broadcasts so the UI reflects the current interlock state
    unsigned long now = millis();
    if (now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS)
    {
        lastTelemetryMs = now;
        wifi.broadcast(build_Telemetry_JSON());
    }
}