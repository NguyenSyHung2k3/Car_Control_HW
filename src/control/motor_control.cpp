#include "../include/control/motor_control.h"

MotorControl::MotorControl(int in1, int in2, int in3, int in4, int ena, int enb)
  : in1(in1), in2(in2), in3(in3), in4(in4), ena(ena), enb(enb), speedA(255), speedB(255) {}

void MotorControl::begin() {
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(ena, OUTPUT);
  pinMode(enb, OUTPUT);

  // PWM setup (for ESP32 use ledc)
  ledcSetup(0, 1000, 8); // Channel 0, 1kHz, 8-bit
  ledcSetup(1, 1000, 8);
  ledcAttachPin(ena, 0);
  ledcAttachPin(enb, 1);

  stop();
}

void MotorControl::setSpeed(int leftSpeed, int rightSpeed) {
  speedA = constrain(leftSpeed, 0, 255);
  speedB = constrain(rightSpeed, 0, 255);
  ledcWrite(0, speedA);
  ledcWrite(1, speedB);
}

void MotorControl::forward() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  ledcWrite(0, speedA);
  ledcWrite(1, speedB);
}

void MotorControl::backward() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  ledcWrite(0, speedA);
  ledcWrite(1, speedB);
}

void MotorControl::left() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  ledcWrite(0, speedA);
  ledcWrite(1, speedB);
}

void MotorControl::right() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  ledcWrite(0, speedA);
  ledcWrite(1, speedB);
}

void MotorControl::stop() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  ledcWrite(0, 0);
  ledcWrite(1, 0);
}