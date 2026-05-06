/**
 * @file WifiCommunication.cpp
 * @brief Implementation of robust TCP communication for TFG Incubator.
 */

#include "WifiCommunication.h"

WifiCommunication::WifiCommunication(const char* ssid, const char* password, uint16_t port) 
    : _ssid(ssid), _password(password), _port(port), _server(port) {
    _lastConnectionAttempt = 0;
}

void WifiCommunication::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    
    Serial.print("Connecting to WiFi: ");
    Serial.println(_ssid);
    
    // Wait for connection (blocking only during setup)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        _server.begin();
    } else {
        Serial.println("\nWiFi Connection Failed. Will retry in background.");
    }
}

void WifiCommunication::handle() {
    // 1. Check WiFi Connection
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - _lastConnectionAttempt > _reconnectInterval) {
            _lastConnectionAttempt = millis();
            WiFi.disconnect();
            WiFi.reconnect();
        }
        return;
    }

    // 2. Manage TCP Client
    if (!_client || !_client.connected()) {
        WiFiClient newClient = _server.available();
        if (newClient) {
            _client = newClient;
            _client.setNoDelay(true); // Optimization for real-time control
            Serial.println("PC Interface connected.");
        }
    }
}

void WifiCommunication::sendTelemetry(String data) {
    if (_client && _client.connected()) {
        _client.println(data); // LabVIEW/Python read until \n
    }
}

String WifiCommunication::getCommand() {
    if (_client && _client.connected() && _client.available()) {
        String cmd = _client.readStringUntil('\n');
        cmd.trim();
        return cmd;
    }
    return "";
}

bool WifiCommunication::isClientConnected() {
    return (_client && _client.connected());
}

String WifiCommunication::getIP() {
    return WiFi.localIP().toString();
}