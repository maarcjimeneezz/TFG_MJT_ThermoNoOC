/**
 * @file test_wifi_communication.cpp
 * @brief Simple test to verify TCP connection and blink LED on connection.
 */

#include <Arduino.h>
#include "WifiCommunication.h"

// Configuration
const char *ssid = "planta2-historic";
const char *password = "planta2historic";
const int port = 5000;
const int LED_PIN = 2;

// Objects
WifiCommunication wifi(ssid, password, port);

void setup_wifi_communication()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // Start with LED off

    // Initialize WiFi
    wifi.begin();

    Serial.println("\n--- WiFi Test Initialized ---");
    Serial.print("Waiting for PC to connect on IP: ");
    Serial.println(WiFi.localIP());
}

void loop_wifi_communication()
{
    // Keep the WiFi and TCP server running
    wifi.handle();

    if (wifi.isClientConnected())
    {
        // 1. BLINK LED: Connection is active
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);

        // 2. SEND TEST DATA: Send a heartbeat to the PC every second
        static unsigned long lastMsg = 0;
        if (millis() - lastMsg > 1000)
        {
            lastMsg = millis();
            wifi.sendTelemetry("STATUS:CONNECTED,Uptime:" + String(millis() / 1000) + "s");
            Serial.println("Sent heartbeat to PC.");
        }

        // 3. LISTEN FOR COMMANDS: If PC sends "OFF", stop the test
        String cmd = wifi.getCommand();
        if (cmd == "OFF")
        {
            Serial.println("PC requested stop. LED will stay ON.");
            digitalWrite(LED_PIN, HIGH);
            while (true)
            {
                delay(1000);
            } // Stop here
        }
    }
    else
    {
        // If not connected, keep LED off or blink very slowly
        digitalWrite(LED_PIN, LOW);
    }
}