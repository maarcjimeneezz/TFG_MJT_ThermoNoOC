/**
 * @file WifiCommunication.h
 * @brief High-level TCP Server for ESP32 to PC (Python/LabVIEW) communication.
 */

#ifndef WIFI_COMMUNICATION_H
#define WIFI_COMMUNICATION_H

#include <WiFi.h>

class WifiCommunication {
private:
    const char* _ssid;
    const char* _password;
    uint16_t _port;
    WiFiServer _server;
    WiFiClient _client;
    unsigned long _lastConnectionAttempt;
    const unsigned long _reconnectInterval = 5000;

public:
    /**
     * @param ssid WiFi Network Name
     * @param password WiFi Password
     * @param port TCP Port (default 5000)
     */
    WifiCommunication(const char* ssid, const char* password, uint16_t port = 5000);

    // Initializes WiFi and starts TCP server
    void begin();
    
    // Non-blocking maintenance of WiFi and Client connections
    void handle();

    // Sends a telemetry string to the PC (adds \n automatically)
    void sendTelemetry(String data);

    // Returns a command string if available, otherwise empty string
    String getCommand();

    // Status helpers
    bool isClientConnected();
    String getIP();
};

#endif