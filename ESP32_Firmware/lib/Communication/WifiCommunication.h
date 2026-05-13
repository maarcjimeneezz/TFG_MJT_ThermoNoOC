/**
 * @file WifiCommunication.h
 * @brief WebSocket Server implementation for real-time bi-directional communication.
 */

#ifndef WIFI_COMMUNICATION_H
#define WIFI_COMMUNICATION_H

#include <WiFi.h>
#include <WebSocketsServer.h> // Ensure you install "WebSockets" by Markus Sattler

class WifiCommunication
{
private:
    WebSocketsServer _webSocket;
    const char *_ssid;
    const char *_password;
    uint16_t _port;

public:
    /**
     * @param ssid WiFi Network Name (AP Name)
     * @param password WiFi Password
     * @param port WebSocket Port (default 5000)
     */
    WifiCommunication(const char *ssid, const char *password, uint16_t port);

    // Initializes WiFi in Access Point mode and starts WebSocket server
    void begin(IPAddress ip, IPAddress gw, IPAddress sn);

    // Must be called in the main loop() to handle heartbeats and incoming data
    void loop();

    // Sends a message to all connected clients (Push telemetry)
    void broadcast(String message);

    // Direct access to the server for advanced event handling
    WebSocketsServer &server() { return _webSocket; }
};

#endif