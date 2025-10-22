#include <Arduino.h>
#include "include/ble_module.h"

// L298N Motor pins
#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12
#define ENA 25
#define ENB 33

MotorControl motor(IN1, IN2, IN3, IN4, ENA, ENB);
BLEController ble(&motor);

void setup() {
  Serial.begin(115200);
  motor.begin();
  motor.setSpeed(200, 200);
  ble.begin();
}

void loop() {
  ble.loop();
}
