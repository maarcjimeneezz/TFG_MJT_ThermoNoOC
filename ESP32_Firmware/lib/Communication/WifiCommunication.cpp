/**
 * @file WifiCommunication.cpp
 * @brief Implementation of WiFi Access Point & JSON Server for TFG Incubator.
 */

#include "WifiCommunication.h"

WifiCommunication::WifiCommunication(const char *ssid, const char *password, uint16_t port)
    : _ssid(ssid), _password(password), _port(port), _server(port) {}

void WifiCommunication::begin(IPAddress ip, IPAddress gw, IPAddress sn)
{
    _local_IP = ip;
    _gateway = gw;
    _subnet = sn;

    // Initialize WiFi in AP mode (Access Point)
    WiFi.softAPConfig(_local_IP, _gateway, _subnet);
    WiFi.softAP(_ssid, _password);

    _server.begin();

    Serial.println("WiFi AP started.");
    Serial.print("AP SSID: ");
    Serial.println(_ssid);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
}

String WifiCommunication::getRequest(WiFiClient &client)
{
    String request = "";
    while (client.available())
    {
        char c = client.read();
        request += c;
    }
    return request;
}

String WifiCommunication::extractJsonValue(String data, String key)
{
    // Simple JSON parser: finds "key":"value" or "key":value
    String searchStr = "\"" + key + "\":";
    int keyPos = data.indexOf(searchStr);

    if (keyPos == -1)
    {
        return "";
    }

    int startPos = keyPos + searchStr.length();

    // Skip whitespace
    while (startPos < data.length() && data[startPos] == ' ')
    {
        startPos++;
    }

    // Check if value is quoted
    if (data[startPos] == '"')
    {
        startPos++;
        int endPos = data.indexOf('"', startPos);
        if (endPos != -1)
        {
            return data.substring(startPos, endPos);
        }
    }
    else
    {
        // Unquoted number or boolean
        int endPos = startPos;
        while (endPos < data.length() && data[endPos] != ',' && data[endPos] != '}' && data[endPos] != ' ')
        {
            endPos++;
        }
        if (endPos > startPos)
        {
            return data.substring(startPos, endPos);
        }
    }

    return "";
}

String WifiCommunication::buildSensorJson(float t1, float h1, float t2, float h2, float uv, float co2, float f1, float f2)
{
    // Build JSON response with sensor data
    String json = "{\"temp1\":" + String(t1, 2) +
                  ",\"hum1\":" + String(h1, 2) +
                  ",\"temp2\":" + String(t2, 2) +
                  ",\"hum2\":" + String(h2, 2) +
                  ",\"uv\":" + String(uv, 2) +
                  ",\"co2\":" + String(co2, 0) +
                  ",\"flow1\":" + String(f1, 2) +
                  ",\"flow2\":" + String(f2, 2) + "}\n";
    return json;
}