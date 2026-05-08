#include <Arduino.h>
#include <WiFi.h>

const char *ssid = "ThermoNoOC";
const char *password = "thermonooc";
IPAddress local_IP(192, 168, 0, 132);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
const uint16_t serverPort = 5000;
WiFiServer server(serverPort);

bool uiConnected = false;
int targetTemperature = 37;
bool uvEnabled[4] = {false, false, false, false};
int uvIntensity[4] = {0, 0, 0, 0};
float pumpFlow[2] = {0.0, 0.0};
String pumpMode[2] = {"continuous", "continuous"};
float pumpFeeding[2] = {0.0, 0.0};
float pumpPause[2] = {0.0, 0.0};
int pumpCycles[2] = {0, 0};

String readClientData(WiFiClient &client)
{
  String payload = "";
  unsigned long start = millis();
  while (client.connected() && millis() - start < 2000)
  {
    while (client.available())
    {
      char c = client.read();
      payload += c;
    }
    if (payload.length() > 0)
    {
      break;
    }
  }
  return payload;
}

String extractJsonString(const String &data, const String &key)
{
  String search = String("\"") + key + String("\"");
  int start = data.indexOf(search);
  if (start < 0)
    return "";
  int colon = data.indexOf(':', start);
  if (colon < 0)
    return "";
  int quote1 = data.indexOf('"', colon + 1);
  if (quote1 < 0)
    return "";
  int quote2 = data.indexOf('"', quote1 + 1);
  if (quote2 < 0)
    return "";
  return data.substring(quote1 + 1, quote2);
}

String extractJsonValue(const String &data, const String &key)
{
  String search = String("\"") + key + String("\"");
  int start = data.indexOf(search);
  if (start < 0)
    return "";
  int colon = data.indexOf(':', start);
  if (colon < 0)
    return "";
  int end = data.indexOf(',', colon + 1);
  if (end < 0)
  {
    end = data.indexOf('}', colon + 1);
  }
  if (end < 0)
  {
    end = data.length();
  }
  String value = data.substring(colon + 1, end);
  value.trim();
  if (value.length() > 0 && value.charAt(0) == '"' && value.charAt(value.length() - 1) == '"')
  {
    value = value.substring(1, value.length() - 1);
  }
  return value;
}

String buildSensorJson()
{
  float temp1 = 37.0;
  float humidity1 = 65.0;
  float temp2 = 36.8;
  float humidity2 = 70.0;
  float uv = 100.0;
  float co2 = 400.0;
  float flow1 = 25.0;
  float flow2 = 30.0;

  String json = "{";
  json += String("\"temp1\":") + String(temp1, 2) + ',';
  json += String("\"humidity1\":") + String(humidity1, 2) + ',';
  json += String("\"temp2\":") + String(temp2, 2) + ',';
  json += String("\"humidity2\":") + String(humidity2, 2) + ',';
  json += String("\"uv\":") + String(uv, 2) + ',';
  json += String("\"co2\":") + String(co2, 2) + ',';
  json += String("\"flow1\":") + String(flow1, 2) + ',';
  json += String("\"flow2\":") + String(flow2, 2);
  json += "}";
  return json;
}

void setup()
{
  Serial.begin(115200);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  server.begin();

  Serial.println("ESP32 ThermoNoOC server started");
  Serial.print("Access point IP: ");
  Serial.println(WiFi.softAPIP());
}

void processClient(WiFiClient &client)
{
  String request = readClientData(client);
  if (request.length() == 0)
  {
    client.stop();
    return;
  }

  String command = extractJsonString(request, "command");
  if (command.length() == 0)
  {
    client.print("{\"status\":\"error\",\"reason\":\"missing command\"}");
    client.stop();
    return;
  }

  uiConnected = true;

  if (command == "get_sensor_data")
  {
    String response = buildSensorJson();
    client.print(response);
  }
  else if (command == "set_temp")
  {
    String value = extractJsonValue(request, "value");
    targetTemperature = value.toInt();
    client.print("{\"status\":\"ok\"}");
  }
  else if (command == "uv_toggle")
  {
    int group = extractJsonValue(request, "group").toInt();
    String enabledValue = extractJsonValue(request, "enabled");
    if (group >= 1 && group <= 4)
    {
      uvEnabled[group - 1] = (enabledValue == "true" || enabledValue == "1");
    }
    client.print("{\"status\":\"ok\"}");
  }
  else if (command == "uv_intensity")
  {
    int group = extractJsonValue(request, "group").toInt();
    int intensity = extractJsonValue(request, "intensity").toInt();
    if (group >= 1 && group <= 4)
    {
      uvIntensity[group - 1] = intensity;
    }
    client.print("{\"status\":\"ok\"}");
  }
  else if (command == "set_pump_flow_and_mode")
  {
    int pump = extractJsonValue(request, "pump").toInt();
    if (pump >= 1 && pump <= 2)
    {
      pumpFlow[pump - 1] = extractJsonValue(request, "flow").toFloat();
      pumpMode[pump - 1] = extractJsonValue(request, "mode");
      pumpFeeding[pump - 1] = extractJsonValue(request, "feeding_time").toFloat();
      pumpPause[pump - 1] = extractJsonValue(request, "pause_time").toFloat();
      pumpCycles[pump - 1] = extractJsonValue(request, "cycles").toInt();
    }
    client.print("{\"status\":\"ok\"}");
  }
  else
  {
    client.print("{\"status\":\"error\",\"reason\":\"unknown command\"}");
  }

  client.stop();
}

void loop()
{
  WiFiClient client = server.available();
  if (client)
  {
    processClient(client);
  }
}