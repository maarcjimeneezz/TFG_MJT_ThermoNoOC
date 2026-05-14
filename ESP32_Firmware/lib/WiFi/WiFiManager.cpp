#include "WiFiManager.h"

WiFiManager::WiFiManager(const char *ssid, const char *password, uint16_t port)
    : _ssid(ssid), _password(password), _port(port), _ws(port) {}

void WiFiManager::begin(IPAddress ip, IPAddress gateway, IPAddress subnet)
{
    WiFi.softAPConfig(ip, gateway, subnet);
    WiFi.softAP(_ssid, _password);
    _ws.begin();

    Serial.print("[WiFi] AP started — SSID: ");
    Serial.print(_ssid);
    Serial.print("  IP: ");
    Serial.print(WiFi.softAPIP());
    Serial.print("  WS port: ");
    Serial.println(_port);
}

void WiFiManager::loop()
{
    _ws.loop();
}

void WiFiManager::broadcast(const String &payload)
{
    _ws.broadcastTXT(payload.c_str());
}