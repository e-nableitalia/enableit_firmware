#ifndef MOTOR_APP_H
#define MOTOR_APP_H

#include <BoardApp.h>
#include <CommandParser.h>
#include <Motor.h>
#include <SMotor.h>

#define MOTOR_APP "motor"

//#define USE_TWO_MOTORS 1

#define MOTOR1_IN1       G38
#define MOTOR1_IN2       G39
#define MOTOR1_ISENSE    -1 // temp disable motor isense G5

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
#define BUS_SERIAL_TX    G6
#define BUS_SERIAL_RX    G7
#define SERVO_ID          1
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
    void cmdGetServoInfo();
    void cmdPingServos(); // <--- nuovo comando
    void cmdScan();
    void cmdSetId();          // <--- nuovo comando per cambiare solo l'id
    void cmdSetBaud();        // <--- nuovo comando per cambiare solo il baudrate
    void cmdUseId();          // <--- imposta l'id di default da usare nei comandi
    void cmdUseRate();        // <--- imposta il baudrate di default da usare nei comandi
    void cmdFeedback();       // <--- stampa i parametri del servo corrente
    void cmdTestMove();       // <--- test movimento avanti/indietro
    void cmdTestSync();       // <--- test movimento sincrono di due servi

    CommandParser<MotorApp> parser;
    int speed = 100;
    bool direction = false;
    int counter = 0;
    int servoId = SERVO_ID;           // <--- variabile dinamica per id
    long servoBaudrate = 1000000;     // <--- variabile dinamica per baudrate (default 1Mbps)
    Motor PQ12Motor[NUM_MOTORS];
    SMotor ST3215Motor;
};

#endif
