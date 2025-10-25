#include "include/wifi_module.h"
#include "include/secrets.h"

// Connect to Wifi network

void connectToWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected, IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed!");
    ESP.restart();
  }
}

// Check if WiFi is connected

bool isWifiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}
