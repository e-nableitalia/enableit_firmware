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
#define CMD_GET_SERVO_INFO "* getservoinfo, leggi info dal servo ST3215"
#define CMD_PING_SERVOS "* pingservos, ping broadcast su tutti i servo"
#define CMD_SCAN "* scan, cerca il servo su vari baudrate"
#define CMD_SET_ID "* setid <id>, cambia solo l'ID del servo (default: 1)"
#define CMD_SET_BAUD "* setbaud <idx>, cambia solo il baudrate del servo (0:1Mbps, 1:500K, 2:250K, 3:128K, 4:115200, 5:76800, 6:57600, 7:38400)"
#define CMD_USE_ID "* useid <id>, imposta l'ID di default per i comandi servo"
#define CMD_USE_RATE "* userate <baud>, imposta il baudrate di default per i comandi servo"
#define CMD_FEEDBACK "* feedback, stampa tutti i parametri del servo corrente"
#define CMD_TESTMOVE "* testmove, muove il servo avanti e indietro in ciclo"
#define CMD_TESTSYNC "* testsync, muove due servi in modo sincrono avanti e indietro"

void MotorApp::enter() {
    log_d("enter MotorApp");
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
    parser.add("getservoinfo", CMD_GET_SERVO_INFO, &MotorApp::cmdGetServoInfo); 
    parser.add("pingservos", CMD_PING_SERVOS, &MotorApp::cmdPingServos);
    parser.add("scan", CMD_SCAN, &MotorApp::cmdScan);
    parser.add("setid", CMD_SET_ID, &MotorApp::cmdSetId);
    parser.add("setbaud", CMD_SET_BAUD, &MotorApp::cmdSetBaud);
    parser.add("useid", CMD_USE_ID, &MotorApp::cmdUseId);
    parser.add("userate", CMD_USE_RATE, &MotorApp::cmdUseRate);
    parser.add("feedback", CMD_FEEDBACK, &MotorApp::cmdFeedback);
    parser.add("testmove", CMD_TESTMOVE, &MotorApp::cmdTestMove);
    parser.add("testsync", CMD_TESTSYNC, &MotorApp::cmdTestSync);
    // Initialize motor with DRV8411 and without speed control
    log_d("Initializing motors");
    log_d("Initializing motor #1");
    PQ12Motor[0].init(MOTOR1_IN1, MOTOR1_IN2, MOTOR1_WIPER, MOTOR1_ISENSE, false);
    #ifdef USE_TWO_MOTORS
    log_d("Initializing motor #2");
    PQ12Motor[1].init(MOTOR2_IN1, MOTOR2_IN2, MOTOR2_WIPER, MOTOR2_ISENSE, false);
    #else
    // Initialize SMotor
    log_d("Initializing SMotor");
    SMotor::static_init(BUS_SERIAL_RX, BUS_SERIAL_TX);
    log_d("SMotor initialized");
    ST3215Motor.init(SERVO_ID);

    // // Ping the servo to check communication
    // log_d("Pinging servo...");
    // bool pingResult = ST3215Motor.Ping(SERVO_ID);
    // if (pingResult) {
    //     log_d("Servo ping successful, id: %d", SERVO_ID);
    // } else {
    //     log_d("Servo ping failed");
    // }
    #endif
}

void MotorApp::leave() {
    log_d("leave MotorApp");
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
    log_d("Motor position: %d", position);
}

void MotorApp::cmdSetSpeed() {
    speed = parser.getInt(1);
    log_d("Motor speed set to: %d", speed);
    PQ12Motor[selectedMotor].speed(speed);
}

void MotorApp::cmdHelp() {
    log_d("Available commands:");
    log_d(CMD_FORWARD);
    log_d(CMD_REVERSE);
    log_d(CMD_GET_POSITION);
    log_d(CMD_SET_SPEED);
    log_d(CMD_HELP);
    log_d(CMD_CURRENT);
    log_d(CMD_STOP);
    log_d(CMD_SET_PIN);
    log_d(CMD_SLEEP);
    log_d(CMD_SELECT_MOTOR);
    log_d(CMD_GET_SERVO_INFO);
    log_d(CMD_PING_SERVOS);
    log_d(CMD_SCAN);
    log_d(CMD_SET_ID);
    log_d(CMD_SET_BAUD);
    log_d(CMD_USE_ID);
    log_d(CMD_USE_RATE);
    log_d(CMD_FEEDBACK);
    log_d(CMD_TESTMOVE);
    log_d(CMD_TESTSYNC);
    log_d("* Opzioni attuali: servoId=%d, servoBaudrate=%ld", servoId, servoBaudrate);
}

void MotorApp::cmdCurrent() {
    int current = PQ12Motor[selectedMotor].getCurrent();
    log_d("Motor current: %d mA", current);
}

void MotorApp::cmdStop() {
    PQ12Motor[selectedMotor].stop("user stop");
    log_d("Motor stopped");
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
        log_d("Invalid pin: %s", pin);
        return;
    }

    pinMode(pinNumber, OUTPUT);
    if (strcmp(state, "high") == 0) {
        digitalWrite(pinNumber, HIGH);
        log_d("Pin %s set to HIGH", pin);
    } else if (strcmp(state, "low") == 0) {
        digitalWrite(pinNumber, LOW);
        log_d("Pin %s set to LOW", pin);
    } else {
        log_d("Invalid state: %s", state);
    }
}

void MotorApp::cmdSleep() {
    PQ12Motor[selectedMotor].sleep();
    log_d("Motor put to sleep");
}

void MotorApp::cmdSelectMotor() {
    int n = parser.getInt(1);
    if (n < 0 || n >= NUM_MOTORS) {
        log_d("Invalid motor index: %d", n);
        return;
    }
    selectedMotor = n;
    log_d("Selected motor: %d", selectedMotor);
}

void MotorApp::cmdGetServoInfo() {
    log_d("Getting servo info for ID %d", servoId);
    int pos = ST3215Motor.ReadPos(servoId);
    int speed = ST3215Motor.ReadSpeed(servoId);
    int load = ST3215Motor.ReadLoad(servoId);
    int voltage = ST3215Motor.ReadVoltage(servoId);
    int temp = ST3215Motor.ReadTemper(servoId);
    int current = ST3215Motor.ReadCurrent(servoId);

    log_d("Pos: %d  Speed: %d  Load: %d  Volt: %fV  Temp: %d°C  Current: %d mA",
        pos, speed, load, voltage / 10.0, temp, current);
}

void MotorApp::cmdPingServos() {
    log_d("Ping su servo ID %d", servoId);
    int result = ST3215Motor.Ping(servoId);
    if (result == servoId) {
        log_d("Ping: il servo con ID %d ha risposto", servoId);
    } else if (result == -1) {
        log_d("Ping: nessun servo ha risposto all'ID %d", servoId);
    } else {
        log_d("Ping: risposta inattesa da ID %d", result);
    }
}

void MotorApp::cmdScan() {
    long bauds[] = {1000000, 500000, 250000, 128000, 115200, 76800, 57600, 38400};
    int nBauds = sizeof(bauds) / sizeof(bauds[0]);
    for (int i = 0; i < nBauds; i++) {
        log_d("Setting baudrate to %d", bauds[i]);
        ST3215Motor.pSerial->begin(bauds[i], SERIAL_8N1, BUS_SERIAL_RX, BUS_SERIAL_TX);
        delay(100);
        int r = ST3215Motor.Ping(0xFE);
        if (r != -1) {
            log_d("Servo found at index %d, baudrate %d, id %d", i, bauds[i], r);
            break;
        }
    }
    log_d("Scan completed");
}

// Sblocca EEPROM, cambia ID a newId
void MotorApp::cmdSetId() {
    int newId = parser.getInt(1);
    if (newId < 0 || newId > 253) {
        log_d("ID non valido: %d (deve essere tra 0 e 253)", newId);
        return;
    }
    log_d("Sblocco EEPROM e cambio ID a %d (ID attuale=%d)", newId, servoId);
    // Sblocca EEPROM
    int res_unlock = ST3215Motor.unLockEprom(servoId);
    if (res_unlock == 1) {
        if (ST3215Motor.getError() == 0) {
            log_d("EEPROM sbloccata con successo");
        } else {
            log_d("Errore sconosciuto durante lo sblocco EEPROM, codice: %d", ST3215Motor.getError());
            return;
        }
    } else {
        log_d("Errore durante lo sblocco EEPROM");
        return;
    }

    // Scrive il nuovo ID
    int res_write = ST3215Motor.writeByte(servoId, SMS_STS_ID, newId);
    if (res_write == 1) {
        if (ST3215Motor.getError() == 0) {
            log_d("ID cambiato con successo a %d", newId);
        } else {
            log_d("Errore sconosciuto durante la scrittura dell'ID, codice: %d", ST3215Motor.getError());
            // Blocca comunque l'EEPROM per sicurezza
            ST3215Motor.LockEprom(newId);
            return;
        }
    } else {
        log_d("Errore durante la scrittura dell'ID");
        // Blocca comunque l'EEPROM per sicurezza
        ST3215Motor.LockEprom(newId);
        return;
    }

    // Blocca di nuovo l'EEPROM
    int res_lock = ST3215Motor.LockEprom(newId);
    if (res_lock == 1) {
        if (ST3215Motor.getError() == 0) {
            log_d("EEPROM bloccata di nuovo");
        } else {
            log_d("Errore sconosciuto durante il blocco EEPROM, codice: %d", ST3215Motor.getError());
        }
    } else {
        log_d("Errore nel blocco dell'EEPROM");
    }
}

// Cambia solo baudrate
void MotorApp::cmdSetBaud() {
    long baudrates[] = {1000000, 500000, 250000, 128000, 115200, 76800, 57600, 38400};
    int nBauds = sizeof(baudrates) / sizeof(baudrates[0]);
    int baudIndex = parser.getInt(1);
    if (baudIndex < 0 || baudIndex >= nBauds) {
        log_d("Indice baudrate non valido: %d", baudIndex);
        return;
    }
    uint8_t baudValue = baudIndex;
    log_d("Sblocco EEPROM per cambio baudrate (ID=%d)", servoId);
    int res_unlock = ST3215Motor.unLockEprom(servoId);
    if (res_unlock == 1) {
        if (ST3215Motor.getError() == 0) {
            log_d("EEPROM sbloccata con successo");
        } else {
            log_d("Errore sconosciuto durante lo sblocco EEPROM, codice: %d", ST3215Motor.getError());
            return;
        }
    } else {
        log_d("Errore durante lo sblocco EEPROM");
        return;
    }

    log_d("Cambio baudrate a %ld bps (ID=%d, N=%d)", baudrates[baudIndex], servoId, baudValue);
    int res_baud = ST3215Motor.writeByte(servoId, SMS_STS_BAUD_RATE, baudValue);
    if (res_baud == 1) {
        if (ST3215Motor.getError() == 0) {
            log_d("Baudrate cambiato con successo a %d bps (N=%d)", baudrates[baudIndex], baudValue);
        } else {
            log_d("Errore sconosciuto durante il cambio baudrate, codice: %d", ST3215Motor.getError());
            // Blocca comunque l'EEPROM per sicurezza
            ST3215Motor.LockEprom(servoId);
            return;
        }
    } else {
        log_d("Errore durante la scrittura del baudrate");
        // Blocca comunque l'EEPROM per sicurezza
        ST3215Motor.LockEprom(servoId);
        return;
    }

    log_d("Riavvio la seriale");
    ST3215Motor.pSerial->begin(baudrates[baudIndex], SERIAL_8N1, BUS_SERIAL_RX, BUS_SERIAL_TX);
    servoBaudrate = baudrates[baudIndex];
    delay(100);
    int ping = ST3215Motor.Ping(servoId);
    if (ping == servoId) {
        log_d("Servo risponde con ID %d e baudrate %d", servoId, servoBaudrate);
    } else {
        log_d("Servo NON risponde dopo cambio baudrate");
    }

    // Blocca di nuovo l'EEPROM
    int res_lock = ST3215Motor.LockEprom(servoId);
    if (res_lock == 1) {
        if (ST3215Motor.getError() == 0) {
            log_d("EEPROM bloccata di nuovo");
        } else {
            log_d("Errore sconosciuto durante il blocco EEPROM, codice: %d", ST3215Motor.getError());
        }
    } else {
        log_d("Errore nel blocco dell'EEPROM");
    }
}

// Imposta l'id di default da usare nei comandi
void MotorApp::cmdUseId() {
    int newId = parser.getInt(1);
    if (newId < 0 || newId > 253) {
        log_d("ID non valido: %d (deve essere tra 0 e 253)", newId);
        return;
    }
    servoId = newId;
    log_d("Impostato servoId di default a %d", servoId);
}

// Imposta il baudrate di default da usare nei comandi
void MotorApp::cmdUseRate() {
    int baudIndex = parser.getInt(1);
    long baudrates[] = {1000000, 500000, 250000, 128000, 115200, 76800, 57600, 38400};
    int nBauds = sizeof(baudrates) / sizeof(baudrates[0]);
    if (baudIndex < 0 || baudIndex >= nBauds) {
        log_d("Indice baudrate non valido: %d (range 0-%d)", baudIndex, nBauds - 1);
        return;
    }
    servoBaudrate = baudrates[baudIndex];
    ST3215Motor.pSerial->begin(servoBaudrate, SERIAL_8N1, BUS_SERIAL_RX, BUS_SERIAL_TX);
    log_d("Impostato servoBaudrate di default a %d (indice %d)", servoBaudrate, baudIndex);
}

void MotorApp::cmdFeedback() {
    int Pos, Speed, Load, Voltage, Temper, Move, Current;
    int res = ST3215Motor.FeedBack(servoId);
    if (res != -1) {
        Pos = ST3215Motor.ReadPos(-1);
        Speed = ST3215Motor.ReadSpeed(-1);
        Load = ST3215Motor.ReadLoad(-1);
        Voltage = ST3215Motor.ReadVoltage(-1);
        Temper = ST3215Motor.ReadTemper(-1);
        Move = ST3215Motor.ReadMove(-1);
        Current = ST3215Motor.ReadCurrent(-1);
        log_d("Position: %d", Pos);
        log_d("Speed: %d", Speed);
        log_d("Load: %d", Load);
        log_d("Voltage: %d", Voltage);
        log_d("Temper: %d", Temper);
        log_d("Move: %d", Move);
        log_d("Current: %d", Current);
    } else {
        log_d("FeedBack err");
    }

    // Letture singole con id esplicito
    Pos = ST3215Motor.ReadPos(servoId);
    if (Pos != -1) {
        log_d("Servo position: %d", Pos);
    } else {
        log_d("read position err");
    }

    Voltage = ST3215Motor.ReadVoltage(servoId);
    if (Voltage != -1) {
        log_d("Servo Voltage: %d", Voltage);
    } else {
        log_d("read Voltage err");
    }

    Temper = ST3215Motor.ReadTemper(servoId);
    if (Temper != -1) {
        log_d("Servo temperature: %d", Temper);
    } else {
        log_d("read temperature err");
    }

    Speed = ST3215Motor.ReadSpeed(servoId);
    if (Speed != -1) {
        log_d("Servo Speed: %d", Speed);
    } else {
        log_d("read Speed err");
    }

    Load = ST3215Motor.ReadLoad(servoId);
    if (Load != -1) {
        log_d("Servo Load: %d", Load);
    } else {
        log_d("read Load err");
    }

    Current = ST3215Motor.ReadCurrent(servoId);
    if (Current != -1) {
        log_d("Servo Current: %d", Current);
    } else {
        log_d("read Current err");
    }

    Move = ST3215Motor.ReadMove(servoId);
    if (Move != -1) {
        log_d("Servo Move: %d", Move);
    } else {
        log_d("read Move err");
    }
    log_d("");
}

void MotorApp::cmdTestMove() {
    log_d("TestMode: muovo il servo avanti e indietro (ID=%d)", servoId);
    for (int i = 0; i < 3; ++i) { // Esegui avanti/indietro 3 volte per evitare loop infinito
        // Avanti
        ST3215Motor.RegWritePosEx(servoId, 4095, 3400, 50);
        // Se vuoi anche il secondo servo, decommenta la riga sotto:
        // ST3215Motor.RegWritePosEx(2, 4095, 3400, 50);
        ST3215Motor.RegWriteAction();
        log_d("Servo %d -> 4095", servoId);
        delay(3000);

        // Indietro
        ST3215Motor.RegWritePosEx(servoId, 0, 3400, 50);
        // Se vuoi anche il secondo servo, decommenta la riga sotto:
        // ST3215Motor.RegWritePosEx(2, 0, 3400, 50);
        ST3215Motor.RegWriteAction();
        log_d("Servo %d -> 0", servoId);
        delay(3000);
    }
    log_d("TestMode completato");
}

void MotorApp::cmdTestSync() {
    log_d("TestSync: muovo due servi in modo sincrono avanti e indietro (ID=1,2)");
    byte ID[2] = {1, 2};
    s16 Position[2];
    u16 Speed[2] = {3400, 3400};
    byte ACC[2] = {50, 50};

    // Avanti
    Position[0] = 3000;
    Position[1] = 3000;
    ST3215Motor.SyncWritePosEx(ID, 2, Position, Speed, ACC);
    log_d("Entrambi i servi -> 3000");
    delay(2000);

    // Indietro
    Position[0] = 100;
    Position[1] = 100;
    ST3215Motor.SyncWritePosEx(ID, 2, Position, Speed, ACC);
    log_d("Entrambi i servi -> 100");
    delay(2000);
}
