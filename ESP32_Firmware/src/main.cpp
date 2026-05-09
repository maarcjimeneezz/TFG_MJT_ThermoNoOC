#include <Arduino.h>
#include <Wire.h>
#include "Incubator.h"
#include "Control.h"
#include "WifiCommunication.h"

// --- Global Objects ---
Incubator inc;
Control ctrl;
WifiCommunication wifi("ThermoNoOC", "thermonooc", 5000);

// --- Network Configuration ---
IPAddress local_IP(192, 168, 0, 132);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

// --- Timing Control ---
unsigned long lastSensorRead = 0;
unsigned long lastTelemetrySend = 0;
unsigned long lastClientCheck = 0;

const unsigned long SENSOR_READ_INTERVAL = 100;  // Read sensors every 100ms
const unsigned long TELEMETRY_INTERVAL = 500;    // Send telemetry every 500ms
const unsigned long CLIENT_CHECK_INTERVAL = 100; // Check for client requests every 100ms

void processCommand(WiFiClient client, String request)
{
  // Extract the main command from JSON
  String command = wifi.extractJsonValue(request, "command");

  // --- Command Routing ---

  if (command == "get_sensor_data")
  {
    // Send actual real-time data read from the sensors
    String response = wifi.buildSensorJson(inc.temp1, inc.hum1, inc.temp2, inc.hum2, inc.uvIndex, inc.co2Percent, inc.flow1, inc.flow2);
    client.print(response);
  }
  else if (command == "set_temp")
  {
    // Set target temperature from UI
    String tempStr = wifi.extractJsonValue(request, "value");
    inc.targetTemperature = tempStr.toFloat();
    Serial.printf("Target Temperature set to: %.2f °C\n", inc.targetTemperature);
    client.print("{\"status\":\"ok\",\"message\":\"Temperature set\"}\n");
  }
  else if (command == "uv_toggle")
  {
    // Toggle UV LED group
    int g = wifi.extractJsonValue(request, "group").toInt() - 1;
    String state = wifi.extractJsonValue(request, "enabled");
    if (g >= 0 && g < 4)
    {
      inc.uvEnabled[g] = (state == "true" || state == "1");
      Serial.printf("UV Group %d: %s\n", g + 1, inc.uvEnabled[g] ? "ON" : "OFF");
      client.print("{\"status\":\"ok\",\"message\":\"UV toggled\"}\n");
    }
    else
    {
      client.print("{\"status\":\"error\",\"message\":\"Invalid group\"}\n");
    }
  }
  else if (command == "uv_intensity")
  {
    // Set UV LED intensity
    int g = wifi.extractJsonValue(request, "group").toInt() - 1;
    int intensity = wifi.extractJsonValue(request, "intensity").toInt();
    if (g >= 0 && g < 4)
    {
      inc.uvIntensity[g] = constrain(intensity, 0, 255);
      Serial.printf("UV Group %d intensity: %d\n", g + 1, inc.uvIntensity[g]);
      client.print("{\"status\":\"ok\",\"message\":\"Intensity updated\"}\n");
    }
    else
    {
      client.print("{\"status\":\"error\",\"message\":\"Invalid group\"}\n");
    }
  }
  else if (command == "set_pump_flow_and_mode")
  {
    // Configure pump flow and mode
    int p = wifi.extractJsonValue(request, "pump").toInt() - 1;
    if (p >= 0 && p < 2)
    {
      inc.pumpConfig[p].flow = wifi.extractJsonValue(request, "flow").toFloat();
      inc.pumpConfig[p].mode = wifi.extractJsonValue(request, "mode");
      inc.pumpConfig[p].feedingTime = wifi.extractJsonValue(request, "feeding_time").toFloat();
      inc.pumpConfig[p].pauseTime = wifi.extractJsonValue(request, "pause_time").toFloat();
      inc.pumpConfig[p].cycles = wifi.extractJsonValue(request, "cycles").toInt();

      Serial.printf("Pump %d config: flow=%.2f µL/min, mode=%s\n", p + 1, inc.pumpConfig[p].flow, inc.pumpConfig[p].mode.c_str());
      client.print("{\"status\":\"ok\",\"message\":\"Pump configured\"}\n");
    }
    else
    {
      client.print("{\"status\":\"error\",\"message\":\"Invalid pump\"}\n");
    }
  }
  else if (command == "stop_all")
  {
    // Emergency stop
    inc.targetTemperature = 25.0;
    inc.setITOPower(0);
    for (int i = 0; i < 4; i++)
    {
      inc.uvEnabled[i] = false;
      inc.uvIntensity[i] = 0;
    }
    for (int i = 0; i < 2; i++)
    {
      inc.pumpConfig[i].flow = 0.0;
    }
    ctrl.setFansSpeed(0);
    Serial.println("System STOPPED");
    client.print("{\"status\":\"ok\",\"message\":\"System stopped\"}\n");
  }
  else if (command == "get_system_status")
  {
    // Return current system status
    float pcbTemp = ctrl.getPCBTemperature();
    String response = "{\"status\":\"ok\",\"pcb_temp\":" + String(pcbTemp, 2) +
                      ",\"target_temp\":" + String(inc.targetTemperature, 2) +
                      ",\"pump1_flow\":" + String(inc.flow1, 2) +
                      ",\"pump2_flow\":" + String(inc.flow2, 2) + "}\n";
    client.print(response);
  }
  else
  {
    // Unknown command
    client.print("{\"status\":\"error\",\"message\":\"Unknown command\"}\n");
  }

  // Close connection to free up the server for the next request
  client.stop();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=== ThermoNoOC ESP32 Firmware ===");
  Serial.println("Initializing system...");

  // Initialize I2C bus
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); // 100kHz I2C speed for stability

  // Initialize hardware modules
  ctrl.begin();
  Serial.println("✓ Control board initialized");

  inc.begin();
  Serial.println("✓ Incubator initialized");

  // Initialize WiFi Access Point
  wifi.begin(local_IP, gateway, subnet);
  Serial.println("✓ WiFi AP started");

  Serial.println("\nSystem Ready: AP 'ThermoNoOC' started.");
  Serial.println("IP: 192.168.0.132");
  Serial.println("Port: 5000");
}

void loop()
{
  unsigned long now = millis();

  // --- 1. READ SENSORS (every 100ms) ---
  if (now - lastSensorRead >= SENSOR_READ_INTERVAL)
  {
    lastSensorRead = now;

    // Read environmental sensors
    inc.readEnvironment();

    // Read PCB temperature for fan control
    float pcbTemp = ctrl.getPCBTemperature();

    // Control cooling fans based on PCB temperature
    if (pcbTemp > 45.0)
    {
      ctrl.setFansSpeed(255); // Full speed if too hot
    }
    else if (pcbTemp > 37.5)
    {
      ctrl.setFansSpeed(192); // 75% speed
    }
    else if (pcbTemp > 30.0)
    {
      ctrl.setFansSpeed(128); // 50% speed
    }
    else
    {
      ctrl.setFansSpeed(64); // 25% speed if cool
    }
  }

  // --- 2. CONTROL ACTUATORS (every 100ms, called with sensors) ---
  if (now - lastSensorRead < 20) // Called right after sensor read
  {
    inc.updateActuators();
  }

  // --- 3. SEND TELEMETRY TO UI (every 500ms) ---
  if (now - lastTelemetrySend >= TELEMETRY_INTERVAL)
  {
    lastTelemetrySend = now;

    // Build telemetry packet
    String telemetry = wifi.buildSensorJson(
        inc.temp1, inc.hum1,
        inc.temp2, inc.hum2,
        inc.uvIndex,
        inc.co2Percent,
        inc.flow1, inc.flow2);

    // Send to all connected clients
    WiFiClient client = wifi.server().available();
    if (client && client.connected())
    {
      client.print(telemetry);
    }
  }

  // --- 4. LISTEN FOR UI COMMANDS (every 100ms) ---
  if (now - lastClientCheck >= CLIENT_CHECK_INTERVAL)
  {
    lastClientCheck = now;

    WiFiClient client = wifi.server().available();
    if (client && client.connected())
    {
      // Read incoming request
      String request = wifi.getRequest(client);

      if (request.length() > 0)
      {
        Serial.println("Received command: " + request);
        processCommand(client, request);
      }
    }
  }
}