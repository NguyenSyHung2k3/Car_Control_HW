#pragma

#include "include/ble_module.h"

#define SERVICE_UUID        "a6decdd8-f8af-49cc-91f9-5a0b43283a6e"
#define CHARACTERISTIC_UUID "d6e61f51-671e-41e1-b865-11c553ef86e2"

class MyServerCallbacks : public BLEServerCallbacks {
  bool *deviceConnectedRef;
public:
  MyServerCallbacks(bool *ref) : deviceConnectedRef(ref) {}
  void onConnect(BLEServer* pServer) override {
    *deviceConnectedRef = true;
    Serial.println("✅ Device connected!");
  }
  void onDisconnect(BLEServer* pServer) override {
    *deviceConnectedRef = false;
    Serial.println("⚠️ Device disconnected!");
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
      Serial.print("📩 Received: ");
      Serial.println(rxValue.c_str());

      if (rxValue == "F") motor->forward();
      else if (rxValue == "B") motor->backward();
      else if (rxValue == "L") motor->left();
      else if (rxValue == "R") motor->right();
      else if (rxValue == "S") motor->stop();
    }
  }
};

BLEController::BLEController(MotorControl *m) : motor(m) {}

void BLEController::begin() {
  BLEDevice::init("ESP32-Car");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks(&deviceConnected));

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->setCallbacks(new MyCallbacks(motor));
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();

  Serial.println("✅ BLE advertising started...");
}

void BLEController::loop() {
  if (deviceConnected) {
    pCharacteristic->setValue("Connected");
    pCharacteristic->notify();
    delay(2000);
  }
}