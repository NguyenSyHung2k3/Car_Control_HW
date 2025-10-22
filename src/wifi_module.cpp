#include "include/wifi_module.h"
#include "include/secrets.h"

// Connect to Wifi network

void connectToWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected, IP: " + WiFi.localIP().toString());
}

// Check if WiFi is connected

bool isWifiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}
