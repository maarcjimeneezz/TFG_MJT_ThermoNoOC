/**
 * @file WifiCommunication.h
 * @brief High-level WiFi Access Point & JSON Server for ESP32 to PC communication.
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

    const char *_ssid;
    const char *_password;
    uint16_t _port;

public:
    /**
     * @param ssid WiFi Network Name (AP Name)
     * @param password WiFi Password
     * @param port TCP Port (default 5000)
     */
    WifiCommunication(const char *ssid, const char *password, uint16_t port);

    // Initializes WiFi in Access Point mode and starts TCP server
    void begin(IPAddress ip, IPAddress gw, IPAddress sn);

    // Main functions to handle incoming client data
    String getRequest(WiFiClient &client);

    // JSON parsers and builders
    String extractJsonValue(String data, String key);
    String buildSensorJson(float t1, float h1, float t2, float h2, float uv, float co2, float f1, float f2);

    // Accessor for the server instance
    WiFiServer &server() { return _server; }
};

#endif