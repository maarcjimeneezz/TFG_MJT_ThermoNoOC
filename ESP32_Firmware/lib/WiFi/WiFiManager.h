#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebSocketsServer.h>

/**
 * @class WiFiManager
 * @brief Manages the WiFi Access Point stack and WebSocket server.
 *
 * Single responsibility: network lifecycle (AP config, connection) and raw
 * telemetry transport (broadcast, receive). Zero knowledge of any domain module.
 *
 * Event handling: register a WebSocket event callback from main.cpp via
 *   wifi.server().onEvent(myHandler);
 */
class WiFiManager
{
private:
    WebSocketsServer _ws;
    const char      *_ssid;
    const char      *_password;
    uint16_t         _port;

public:
    WiFiManager(const char *ssid, const char *password, uint16_t port = 5000);

    /** Configures AP and starts the WebSocket server. Call once in setup(). */
    void begin(IPAddress ip, IPAddress gateway, IPAddress subnet);

    /** Processes WebSocket heartbeats and incoming frames. Call every loop(). */
    void loop();

    /** Broadcasts a UTF-8 payload to all connected clients. */
    void broadcast(const String &payload);

    /** Exposes the server so main.cpp can register onEvent callbacks. */
    WebSocketsServer &server() { return _ws; }
};

#endif // WIFI_MANAGER_H
