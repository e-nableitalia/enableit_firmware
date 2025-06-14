//
// Motor: DRV8833 / DRV8835 / DRV8411 motor driver
//
// Author: A.Navatta

#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef ARDUINO_M5Stack_ATOMS3

//#define DRIVER_DRV8833
//#define DRIVER_DRV8835
#define DRIVER_DRV8411

#define MOTOR_IN1       G5
#define MOTOR_IN2       G6

#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
#define MOTOR_ENABLE    Gx
#define MOTOR_FAULT     Gx
#endif

// Atom S3 doesn't have enough pins to read the motor position
// so motor position is disabled

// WIPER is the potentiometer to read the motor position
// MOTOR_POSITION_PROTECTION is the flag to enable the protection
// MOTOR_WIPER is the pin to read the motor position
//#define MOTOR_POSITION_PROTECTION   1
//#define MOTOR_WIPER     G38

// MOTOR_CURRENT_PROTECTION is the flag to enable the current protection
// MOTOR_ISENSE is the pin to read the motor current
#define MOTOR_CURRENT_PROTECTION     1
#define MOTOR_ISENSE    G7
#else
// need to be defined per board specific
#pragma message "Please define the motor pins for your board"
#endif

#define MOTOR_SPEED_BIT (uint8_t)10
#define MOTOR_SPEED_MAX (1 << MOTOR_SPEED_BIT) - 1
#define MOTOR_PWM_FREQ  78000

// 96 is safety threshold
#define MOTOR_LOW_THRESHOLD 1700
#define MOTOR_HIGH_THRESHOLD 3950
#define MOTOR_CURRENT_THRESHOLD 2100

#define MOTOR_FASTDECAY_MODE    0

enum MOTOR_DIRECTION { FORWARD, REVERSE, STOP };

class Motor {
public:
    Motor();
    void init(bool enableSpeed = false);
    void speed(int speed);
    void forward(int speed);
    void reverse(int speed);
    void stop(String message);
    void sleep();
    void wakeup();
    void poll();

    inline int getPosition() { return position; };
    inline int getCurrent() { return current; };

private:
    int position;
    int current;
    int current_max;
    bool speedControl;
    MOTOR_DIRECTION direction;
};

extern Motor PQ12Motor;

#endif