/**
 * @file WifiCommunication.cpp
 * @brief Implementation of WebSocket-based communication for the ThermoNoOC project.
 */

#include "WifiCommunication.h"

WifiCommunication::WifiCommunication(const char *ssid, const char *password, uint16_t port)
    : _ssid(ssid), _password(password), _port(port), _webSocket(port) {}

void WifiCommunication::begin(IPAddress ip, IPAddress gw, IPAddress sn)
{
    // Setup Access Point
    WiFi.softAPConfig(ip, gw, sn);
    WiFi.softAP(_ssid, _password);

    // Start WebSocket Server
    _webSocket.begin();

    Serial.println("WebSocket Server initialized.");
    Serial.print("SSID: ");
    Serial.println(_ssid);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("Port: ");
    Serial.println(_port);
}

void WifiCommunication::loop()
{
    _webSocket.loop();
}

void WifiCommunication::broadcast(String message)
{
    // Sends the string to every connected UI instance
    _webSocket.broadcastTXT(message);
}