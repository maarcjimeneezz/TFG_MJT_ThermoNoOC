/**
 * @file test_wifi_communication.cpp
 * @brief Minimalist test to verify WebSocket connection and LED control.
 */

#include <Arduino.h>
#include "WiFiManager.h"
#include <WebSocketsServer.h>

// --- Global Objects ---
// Creating the AP 'ThermoNoOC' on Port 5000
WiFiManager wifi("ThermoNoOC", "thermonooc", 5000);

// --- Network Configuration ---
IPAddress local_IP(192, 168, 0, 132);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

// --- Hardware Pins ---
const int DEBUG_LED = 2; // Internal LED for most ESP32 DevKits

/**
 * @brief WebSocket Event Handler
 * This function triggers automatically when the PC UI sends data.
 */
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_CONNECTED:
        Serial.printf("[%u] UI Connected!\n", num);
        break;

    case WStype_DISCONNECTED:
        Serial.printf("[%u] UI Disconnected\n", num);
        break;

    case WStype_TEXT:
    {
        String msg = (char *)(payload);
        Serial.printf("[%u] Message from UI: %s\n", num, msg.c_str());

        // Check if the command is "blink"
        if (msg.indexOf("blink") != -1)
        {
            Serial.println("Action: Blinking LED...");
            for (int i = 0; i < 5; i++)
            {
                digitalWrite(DEBUG_LED, HIGH);
                delay(200);
                digitalWrite(DEBUG_LED, LOW);
                delay(200);
            }

            // Send an acknowledgment back to the UI
            wifi.server().sendTXT(num, "{\"status\":\"success\", \"event\":\"blink_done\"}");
        }
    }
    break;

    case WStype_BIN:
        break;

    default:
        break;
    }
}

void setup_wifi_communication()
{
    // 1. Start Serial for debugging
    Serial.begin(115200);
    pinMode(DEBUG_LED, OUTPUT);

    // 2. Initialize the WiFi Access Point and WebSocket Server
    // This uses your WifiCommunication.cpp logic
    wifi.begin(local_IP, gateway, subnet);

    // 3. Attach the event handler to the server
    wifi.server().onEvent(onWebSocketEvent);

    Serial.println("\n--- TEST MODE READY ---");
    Serial.println("1. Connect your PC to WiFi: 'ThermoNoOC'");
    Serial.println("2. Open your WebSocket client on 192.168.0.132:5000");
}

void loop_wifi_communication()
{
    // Crucial: This handles the WebSocket protocol background tasks
    wifi.loop();
}