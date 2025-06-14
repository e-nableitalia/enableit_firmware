#ifndef MOTOR_APP_H
#define MOTOR_APP_H

#include <BoardApp.h>
#include <CommandParser.h>

#define MOTOR_APP "motor"

class MotorApp : public BoardApp {
public:
    void enter();
    void leave();
    void process();

    const char *name() { return MOTOR_APP; }

private:
    void cmdForward();
    void cmdReverse();
    void cmdGetPosition();
    void cmdSetSpeed();
    void cmdHelp();
    void cmdCurrent();
    void cmdStop();
    void cmdSetPin(); // New command for setting pin state
    void cmdSleep();

    CommandParser<MotorApp> parser;
    int speed = 100;
    bool direction = false;
    int counter = 0;
};

#endif
