#ifndef MOTOR_APP_H
#define MOTOR_APP_H

#include <BoardApp.h>
#include <CommandParser.h>
#include <Motor.h>

#define MOTOR_APP "motor"

#define USE_TWO_MOTORS 1

#define MOTOR1_IN1       G38
#define MOTOR1_IN2       G39
#define MOTOR1_ISENSE    G5

#ifdef USE_TWO_MOTORS
#define NUM_MOTORS 2
#define MOTOR1_WIPER     -1
#define MOTOR2_IN1       G6
#define MOTOR2_IN2       G7
#define MOTOR2_WIPER     -1
#define MOTOR2_ISENSE    G8
#else
#define NUM_MOTORS 1
#define MOTOR1_WIPER     G8
#endif

class MotorApp : public BoardApp {
public:
    void enter();
    void leave();
    void process();

    const char *name() { return MOTOR_APP; }

private:
    int selectedMotor = 0;
    void cmdForward();
    void cmdReverse();
    void cmdGetPosition();
    void cmdSetSpeed();
    void cmdHelp();
    void cmdCurrent();
    void cmdStop();
    void cmdSetPin(); // New command for setting pin state
    void cmdSleep();
    void cmdSelectMotor();

    CommandParser<MotorApp> parser;
    int speed = 100;
    bool direction = false;
    int counter = 0;
    Motor PQ12Motor[NUM_MOTORS];
};

#endif
