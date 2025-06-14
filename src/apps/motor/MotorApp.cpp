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
    parser.add("sleep", CMD_SLEEP, &MotorApp::cmdSleep); // Aggiunto comando sleep
    // Initialize motor with DRV8411 and without speed control
    PQ12Motor.init(true);
    // ...initialize other commands if needed...
}

void MotorApp::leave() {
    DBG("leave MotorApp");
    // ...cleanup if needed...
}

void MotorApp::process() {
    parser.poll();
    // Poll the PQ12 motor
    PQ12Motor.poll();
}

void MotorApp::cmdForward() {
    PQ12Motor.forward(speed);
}

void MotorApp::cmdReverse() {
    PQ12Motor.reverse(speed);
}

void MotorApp::cmdGetPosition() {
    int position = PQ12Motor.getPosition();
    DBG("Motor position: %d", position);
}

void MotorApp::cmdSetSpeed() {
    speed = parser.getInt(1);
    DBG("Motor speed set to: %d", speed);
    PQ12Motor.speed(speed);
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
}

void MotorApp::cmdCurrent() {
    int current = PQ12Motor.getCurrent();
    DBG("Motor current: %d mA", current);
}

void MotorApp::cmdStop() {
    PQ12Motor.stop("user stop");
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
    PQ12Motor.sleep();
    DBG("Motor put to sleep");
}
