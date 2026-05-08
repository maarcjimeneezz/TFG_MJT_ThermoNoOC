#include <Arduino.h>
#include "Incubator.h"
#include "WifiCommunication.h"

// --- Global Objects ---
Incubator inc;
WifiCommunication wifi("ThermoNoOC", "thermonooc", 5000);

// --- Network Configuration ---
IPAddress local_IP(192, 168, 0, 132);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

void setup()
{
  Serial.begin(115200);

  // Initialize hardware and sensors
  inc.begin();

  // Initialize WiFi Access Point
  wifi.begin(local_IP, gateway, subnet);
  Serial.println("System Ready: AP ThermoNoOC started.");
}

void processClient(WiFiClient client)
{
  // Read the incoming message from Python/UI
  String request = wifi.getRequest(client);
  if (request.length() == 0)
    return;

  // Extract the main command
  String command = wifi.extractJsonValue(request, "command");

  // --- Command Routing ---

  if (command == "get_sensor_data")
  {
    // Send actual real-time data read from the sensors
    String response = wifi.buildSensorJson(inc.temp1, inc.hum1, inc.temp2, inc.hum2, inc.uvIndex, (float)inc.co2PPM, inc.flow1, inc.flow2);
    client.print(response);
  }
  else if (command == "set_temp")
  {
    inc.targetTemperature = wifi.extractJsonValue(request, "value").toFloat();
    client.print("{\"status\":\"ok\"}");
  }
  else if (command == "uv_toggle")
  {
    int g = wifi.extractJsonValue(request, "group").toInt() - 1;
    String state = wifi.extractJsonValue(request, "enabled");
    inc.uvEnabled[g] = (state == "true" || state == "1");
    client.print("{\"status\":\"ok\"}");
  }
  else if (command == "uv_intensity")
  {
    int g = wifi.extractJsonValue(request, "group").toInt() - 1;
    inc.uvIntensity[g] = wifi.extractJsonValue(request, "intensity").toInt();
    client.print("{\"status\":\"ok\"}");
  }
  else if (command == "set_pump_flow_and_mode")
  {
    int p = wifi.extractJsonValue(request, "pump").toInt() - 1;
    // Update the pump configuration in the incubator object
    inc.pumpConfig[p].flow = wifi.extractJsonValue(request, "flow").toFloat();
    inc.pumpConfig[p].mode = wifi.extractJsonValue(request, "mode");
    inc.pumpConfig[p].feedingTime = wifi.extractJsonValue(request, "feeding_time").toFloat();
    inc.pumpConfig[p].pauseTime = wifi.extractJsonValue(request, "pause_time").toFloat();
    inc.pumpConfig[p].cycles = wifi.extractJsonValue(request, "cycles").toInt();

    client.print("{\"status\":\"ok\"}");
  }

  // Close connection to free up the server for the next request
  client.stop();
}

void loop()
{
  // 1. Always read sensors continuously (Temperature, UV, Flow)
  inc.readEnvironment();

  // 2. Control hardware (Adjust ITO heater, update LEDs and Pumps)
  inc.updateActuators();

  // 3. Listen for UI requests
  WiFiClient client = wifi.server().available();
  if (client)
  {
    processClient(client);
  }
}