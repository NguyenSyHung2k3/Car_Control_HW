#pragma

#include "include/ble_module.h"

class MyServerCallbacks : public BLEServerCallbacks {
  bool *deviceConnectedRef;
public:
  MyServerCallbacks(bool *ref) : deviceConnectedRef(ref) {}
  void onConnect(BLEServer* pServer) override {
    *deviceConnectedRef = true;
  }
  void onDisconnect(BLEServer* pServer) override {
    *deviceConnectedRef = false;
    BLEDevice::startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  MotorControl *motor;
public:
  MyCallbacks(MotorControl *m) : motor(m) {}
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      // Direct motor control through BLE
      if (rxValue == "F") {
        motor->forward();
        Serial.println("[BLE] Motor forward");
      } else if (rxValue == "D") {
        motor->backward();
        Serial.println("[BLE] Motor backward");
      } else if (rxValue == "L") {
        motor->left();
        Serial.println("[BLE] Motor left");
      } else if (rxValue == "R") {
        motor->right();
        Serial.println("[BLE] Motor right");
      } else if (rxValue == "S") {
        motor->stop();
        Serial.println("[BLE] Motor stop");
      } else {
        Serial.printf("[BLE] Unknown command: %s\n", rxValue.c_str());
      }
    }
  }
};

BLEController::BLEController(MotorControl *m) : motor(m) {}

void BLEController::begin() {
  Serial.println("Initializing BLE...");
  
  BLEDevice::init("ESP32-Car");
  Serial.println("BLE Device initialized");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks(&deviceConnected));
  Serial.println("BLE Server created");

  BLEService *pService = pServer->createService(SERVICE_UUID);
  Serial.println("BLE Service created");

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  Serial.println("BLE Characteristic created");

  pCharacteristic->setCallbacks(new MyCallbacks(motor));
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  Serial.println("BLE Service started");

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
  Serial.println("BLE advertising started...");
}

void BLEController::loop() {
  // Non-blocking BLE loop to prevent watchdog reset
  static unsigned long lastNotify = 0;
  if (deviceConnected && millis() - lastNotify >= 2000) {
    pCharacteristic->setValue("Connected");
    pCharacteristic->notify();
    lastNotify = millis();
  }
}