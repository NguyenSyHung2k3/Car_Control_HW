#pragma

#include <Arduino.h>

class MotorControl {
  private:
    int in1, in2, in3, in4;
    int ena, enb;
    int speedA, speedB;

  public:
    MotorControl(int in1, int in2, int in3, int in4, int ena, int enb);
    void begin();
    void forward();
    void backward();
    void left();
    void right();
    void stop();
    void setSpeed(int leftSpeed, int rightSpeed);
};

