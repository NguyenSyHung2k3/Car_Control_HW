#include <Arduino.h>
#include "include/ble_module.h"
#include "include/mqtt_module.h"
#include "include/wifi_module.h"
#include "include/utils/ota.h"
#include "include/secrets.h"

MotorControl motor(IN1, IN2, IN3, IN4, ENA, ENB);
BLEController ble(&motor);
MqttClient* mqttClient = nullptr;
OTAManager* otaManager = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting ESP32 IoT Car...");
  
  // Initialize motor first
  motor.begin();
  motor.setSpeed(200, 200);
  Serial.println("Motor initialized");
  
  // Initialize BLE
  ble.begin();
  Serial.println("BLE initialized");
  
  // Connect to WiFi
  connectToWifi();
  Serial.println("WiFi connected");
  
  // Initialize MQTT Client
  mqttClient = MqttClient::getInstance();
  mqttClient->connect();
  mqttClient->startContinuousListening();
  mqttClient->connectionPublish();
  mqttClient->connectionSubscribe();
  mqttClient->mqttMessageCallback();
  mqttClient->otaSubscribe();
  Serial.println("MQTT initialized");
  
  // Initialize OTA Manager
  otaManager = new OTAManager(OTA_SERVER_URL);
  otaManager->begin();
  Serial.println("OTA Manager initialized");
  
  Serial.println("Setup completed!");
}

void loop() {
  // Handle BLE (highest priority)
  ble.loop();
  
  // Handle MQTT messages
  if (mqttClient) {
    mqttClient->loop();
  }
  
  // Handle OTA updates
  if (otaManager) {
    otaManager->loop();
  }
  
  // Small delay to prevent watchdog reset
  delay(10);
}
