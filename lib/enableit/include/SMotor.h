// SMotor: gestione servo Waveshare SC Servo
// Author: A.Navatta

#ifndef SMOTOR_H
#define SMOTOR_H

#include <Arduino.h>

#include <SCServo.h>

class SMotor : public SMS_STS {
public:
    SMotor();
    void init(uint8_t servoId);
    static void static_init(int sRX, int sTX);
    void speed(int speed);
    void move(int position);
    void forward(int speed);
    void reverse(int speed);
    void stop(String message);
    void sleep();
    void wakeup();
    void poll();
    int getPosition();
    int getCurrent();
    int getError();
private:
    uint8_t id;
    static HardwareSerial* scSerial;
    int lastPosition;
};

#endif
