/**
 * @file test_wifi_communication.cpp
 * @brief connectivity test: Blinks LED only when a PC connects to the AP.
 */

// After Uploading the file to the ESP32 Board, go to file ../UI/pc_test_connection.py and execute it.
// This file will connect the PC to the ESP32 Board, make the LED blink for 10 seconds
#include <Arduino.h>
#include "WifiCommunication.h"

// --- AP Configuration ---
const char *ssid = "ThermoNoOC";
const char *password = "thermonooc";
const int port = 5000;
const int LED_PIN = 2; // Internal LED (GPIO 2 on most ESP32)

// --- Network IP Configuration ---
IPAddress local_IP(192, 168, 0, 132);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

// Initialize the communication object
WifiCommunication wifi(ssid, password, port);

void setup_wifi_communication()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // LED off by default

    // Initialize WiFi Access Point
    wifi.begin(local_IP, gateway, subnet);

    Serial.println("\n--- WIFI AP CONNECTIVITY TEST ---");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("IP:   ");
    Serial.println(local_IP);
    Serial.println("Waiting for PC connection...");
}

void loop_wifi_communication()
{
    // Check if a client (PC) is connecting to the server
    WiFiClient client = wifi.server().available();

    if (client)
    {
        Serial.println("[!] PC Connected! Blinking LED...");

        // While the PC is connected, blink the LED
        while (client.connected())
        {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);

            // Check if there's any data just to clear the buffer
            while (client.available())
            {
                client.read();
            }
        }

        // When PC disconnects
        client.stop();
        digitalWrite(LED_PIN, LOW);
        Serial.println("[!] PC Disconnected. LED Off.");
    }
}