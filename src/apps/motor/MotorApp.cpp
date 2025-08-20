#include <Motor.h>
#include "MotorApp.h"

#define CMD_FORWARD "* forward, move motor forward"
#define CMD_REVERSE "* reverse, move motor reverse"
#define CMD_GET_POSITION "* getposition, get motor position"
#define CMD_SET_SPEED "* setspeed, set motor speed"
#define CMD_HELP "* help, list available commands"
#define CMD_CURRENT "* current, get motor current"
#define CMD_STOP "* stop, stop the motor"
#define CMD_SET_PIN "* setpin <pin> <state>, set pin state (high/low)"
#define CMD_SLEEP "* sleep, put the motor in sleep mode"
#define CMD_SELECT_MOTOR "* selectmotor <n>, select active motor (0/1)"

void MotorApp::enter() {
    DBG("enter MotorApp");
    parser.init(this);
    parser.add("forward", CMD_FORWARD, &MotorApp::cmdForward);
    parser.add("reverse", CMD_REVERSE, &MotorApp::cmdReverse);
    parser.add("getposition", CMD_GET_POSITION, &MotorApp::cmdGetPosition);
    parser.add("setspeed", CMD_SET_SPEED, &MotorApp::cmdSetSpeed);
    parser.add("help", CMD_HELP, &MotorApp::cmdHelp);
    parser.add("current", CMD_CURRENT, &MotorApp::cmdCurrent);
    parser.add("stop", CMD_STOP, &MotorApp::cmdStop);
    parser.add("setpin", CMD_SET_PIN, &MotorApp::cmdSetPin);
    parser.add("sleep", CMD_SLEEP, &MotorApp::cmdSleep);
    parser.add("selectmotor", CMD_SELECT_MOTOR, &MotorApp::cmdSelectMotor);
    // Initialize motor with DRV8411 and without speed control
    DBG("Initializing motors");
    DBG("Initializing motor #1");
    PQ12Motor[0].init(MOTOR1_IN1, MOTOR1_IN2, MOTOR1_WIPER, MOTOR1_ISENSE, false);
    #ifdef USE_TWO_MOTORS
    DBG("Initializing motor #2");
    PQ12Motor[1].init(MOTOR2_IN1, MOTOR2_IN2, MOTOR2_WIPER, MOTOR2_ISENSE, false);
    #endif
    // ...initialize other commands if needed...
}

void MotorApp::leave() {
    DBG("leave MotorApp");
    // ...cleanup if needed...
}

void MotorApp::process() {
    parser.poll();
    PQ12Motor[selectedMotor].poll();
}

void MotorApp::cmdForward() {
    PQ12Motor[selectedMotor].forward(speed);
}

void MotorApp::cmdReverse() {
    PQ12Motor[selectedMotor].reverse(speed);
}

void MotorApp::cmdGetPosition() {
    int position = PQ12Motor[selectedMotor].getPosition();
    DBG("Motor position: %d", position);
}

void MotorApp::cmdSetSpeed() {
    speed = parser.getInt(1);
    DBG("Motor speed set to: %d", speed);
    PQ12Motor[selectedMotor].speed(speed);
}

void MotorApp::cmdHelp() {
    DBG("Available commands:");
    DBG(CMD_FORWARD);
    DBG(CMD_REVERSE);
    DBG(CMD_GET_POSITION);
    DBG(CMD_SET_SPEED);
    DBG(CMD_HELP);
    DBG(CMD_CURRENT);
    DBG(CMD_STOP);
    DBG(CMD_SET_PIN);
    DBG(CMD_SLEEP);
    DBG(CMD_SELECT_MOTOR);
}

void MotorApp::cmdCurrent() {
    int current = PQ12Motor[selectedMotor].getCurrent();
    DBG("Motor current: %d mA", current);
}

void MotorApp::cmdStop() {
    PQ12Motor[selectedMotor].stop("user stop");
    DBG("Motor stopped");
}

void MotorApp::cmdSetPin() {
    const char *pin = parser.getString(1);
    const char *state = parser.getString(2);

    int pinNumber = -1;
    if (strcmp(pin, "G5") == 0) pinNumber = 5;
    else if (strcmp(pin, "G6") == 0) pinNumber = 6;
    else if (strcmp(pin, "G7") == 0) pinNumber = 7;
    else if (strcmp(pin, "G8") == 0) pinNumber = 8;
    else if (strcmp(pin, "G38") == 0) pinNumber = 38;
    else if (strcmp(pin, "G39") == 0) pinNumber = 39;

    if (pinNumber == -1) {
        DBG("Invalid pin: %s", pin);
        return;
    }

    pinMode(pinNumber, OUTPUT);
    if (strcmp(state, "high") == 0) {
        digitalWrite(pinNumber, HIGH);
        DBG("Pin %s set to HIGH", pin);
    } else if (strcmp(state, "low") == 0) {
        digitalWrite(pinNumber, LOW);
        DBG("Pin %s set to LOW", pin);
    } else {
        DBG("Invalid state: %s", state);
    }
}

void MotorApp::cmdSleep() {
    PQ12Motor[selectedMotor].sleep();
    DBG("Motor put to sleep");
}

void MotorApp::cmdSelectMotor() {
    int n = parser.getInt(1);
    if (n < 0 || n >= NUM_MOTORS) {
        DBG("Invalid motor index: %d", n);
        return;
    }
    selectedMotor = n;
    DBG("Selected motor: %d", selectedMotor);
}
