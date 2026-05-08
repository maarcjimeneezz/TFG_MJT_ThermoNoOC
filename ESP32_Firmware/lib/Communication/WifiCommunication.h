/**
 * @file WifiCommunication.h
 * @brief High-level TCP Server for ESP32 to PC (Python/LabVIEW) communication.
 */

#ifndef WIFI_COMMUNICATION_H
#define WIFI_COMMUNICATION_H

#include <WiFi.h>

class WifiCommunication
{
private:
    WiFiServer _server;
    IPAddress _local_IP;
    IPAddress _gateway;
    IPAddress _subnet;

public:
    /**
     * @param ssid WiFi Network Name
     * @param password WiFi Password
     * @param port TCP Port (default 5000)
     */
    WifiCommunication(const char *ssid, const char *password, uint16_t port);

    // Initializes WiFi and starts TCP server
    void begin(IPAddress ip, IPAddress gw, IPAddress sn);

    // Main loop to handle incoming client connections and requests
    String getRequest(WiFiClient &client);
    String extractJsonValue(String data, String key);
    String buildSensorJson(float t1, float h1, float t2, float h2, float uv, float co2, float f1, float f2);

    // Accessor for the server instance
    WiFiServer &server() { return _server; }
};

#endif