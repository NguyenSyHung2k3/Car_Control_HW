#pragma

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "control/motor_control.h"

class BLEController {
  private:
    BLECharacteristic *pCharacteristic;
    bool deviceConnected = false;
    MotorControl *motor;
    
  public:
    BLEController(MotorControl *motor);
    void begin();
    void loop();
};