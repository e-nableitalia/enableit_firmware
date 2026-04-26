#include <Motor.h>
#include <BoardApp.h>
#include <BoardManager.h>
#include <Board.h>
#include <WiFi.h>
#include "MotorApp.h"

BOARDAPP_INSTANCE(MotorApp);

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
#define CMD_OTA "* ota, switch to OTA update app"
#define CMD_BOOT "* boot, switch to bootloader for board configuration"
#define CMD_REBOOT "* reboot, reboot the board"

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
    parser.add("ota", CMD_OTA, &MotorApp::cmdOta);
    parser.add("boot", CMD_BOOT, &MotorApp::cmdBoot);
    parser.add("reboot", CMD_REBOOT, &MotorApp::cmdReboot);
    // Initialize H-bridge motors (only if present on this board)
    #if NUM_MOTORS > 0
    log_d("Initializing motors");
    log_d("Initializing motor #1");
    PQ12Motor[0].init(MOTOR1_IN1, MOTOR1_IN2, MOTOR1_WIPER, MOTOR1_ISENSE, false);
    #ifdef USE_TWO_MOTORS
    log_d("Initializing motor #2");
    PQ12Motor[1].init(MOTOR2_IN1, MOTOR2_IN2, MOTOR2_WIPER, MOTOR2_ISENSE, false);
    #endif
    #endif
    // Initialize serial servo bus (present whenever USE_TWO_MOTORS is not set)
    #ifndef USE_TWO_MOTORS
    log_d("Initializing STMotor on TX=%d RX=%d", (int)BUS_SERIAL_TX, (int)BUS_SERIAL_RX);
    STMotor::static_init(BUS_SERIAL_RX, BUS_SERIAL_TX);
    log_d("STMotor initialized");
    ST3215Motor.init(SERVO_ID);

    #endif // !USE_TWO_MOTORS

    // ── Build generic motor registry ──────────────────────────────────────────
    // Order: servo first (slot 0), then H-bridge motors
    _motorCount = 0;
    #ifndef USE_TWO_MOTORS
    _motors[_motorCount++] = &ST3215Motor;
    #endif
    #if NUM_MOTORS > 0
    for (int i = 0; i < NUM_MOTORS; i++)
        _motors[_motorCount++] = &PQ12Motor[i];
    #endif
    selectedMotor = 0;
    OUT("Motors registered: %d", _motorCount);
    for (int i = 0; i < _motorCount; i++)
        log_d("  [%d] %s", i, _motors[i]->getType());

    // Display init
    enableit::board.getDisplay().clear();
    enableit::board.getDisplay().setTextSize(2);
    enableit::board.getDisplay().setTitle("MotorTestApp");
    enableit::board.getDisplay().setTextSize(1);
    enableit::board.getDisplay().setLine(1, "IP: " + WiFi.localIP().toString());
}

void MotorApp::leave() {
    log_d("leave MotorApp");
    // ...cleanup if needed...
}

void MotorApp::process() {
    parser.poll();
    for (int i = 0; i < _motorCount; i++)
        _motors[i]->poll();
}

void MotorApp::cmdForward() {
    if (_motorCount == 0) { OUT("No motor configured"); return; }
    _motors[selectedMotor]->forward(speed);
    OUT("[%s] forward speed=%d", _motors[selectedMotor]->getType(), speed);
}

void MotorApp::cmdReverse() {
    if (_motorCount == 0) { OUT("No motor configured"); return; }
    _motors[selectedMotor]->reverse(speed);
    OUT("[%s] reverse speed=%d", _motors[selectedMotor]->getType(), speed);
}

void MotorApp::cmdGetPosition() {
    if (_motorCount == 0) { OUT("No motor configured"); return; }
    int position = _motors[selectedMotor]->getPosition();
    OUT("[%s] position: %d", _motors[selectedMotor]->getType(), position);
}

void MotorApp::cmdSetSpeed() {
    speed = parser.getInt(1);
    if (_motorCount == 0) { OUT("No motor configured"); return; }
    _motors[selectedMotor]->setSpeed(speed);
    OUT("[%s] speed set to: %d", _motors[selectedMotor]->getType(), speed);
}

void MotorApp::cmdHelp() {
    OUT("Available commands:");
    OUT(CMD_FORWARD);
    OUT(CMD_REVERSE);
    OUT(CMD_GET_POSITION);
    OUT(CMD_SET_SPEED);
    OUT(CMD_HELP);
    OUT(CMD_CURRENT);
    OUT(CMD_STOP);
    OUT(CMD_SET_PIN);
    OUT(CMD_SLEEP);
    OUT(CMD_SELECT_MOTOR);
    OUT(CMD_GET_SERVO_INFO);
    OUT(CMD_PING_SERVOS);
    OUT(CMD_SCAN);
    OUT(CMD_SET_ID);
    OUT(CMD_SET_BAUD);
    OUT(CMD_USE_ID);
    OUT(CMD_USE_RATE);
    OUT(CMD_FEEDBACK);
    OUT(CMD_TESTMOVE);
    OUT(CMD_TESTSYNC);
    OUT(CMD_OTA);
    OUT(CMD_BOOT);
    OUT(CMD_REBOOT);
    OUT("* Opzioni attuali: servoId=%d, servoBaudrate=%d", servoId, servoBaudrate);
}

void MotorApp::cmdCurrent() {
    if (_motorCount == 0) { OUT("No motor configured"); return; }
    int current = _motors[selectedMotor]->getCurrent();
    OUT("[%s] current: %d mA", _motors[selectedMotor]->getType(), current);
}

void MotorApp::cmdStop() {
    if (_motorCount == 0) { OUT("No motor configured"); return; }
    _motors[selectedMotor]->stop();
    OUT("[%s] stopped", _motors[selectedMotor]->getType());
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
        OUT("Invalid pin: %s", pin);
        return;
    }

    pinMode(pinNumber, OUTPUT);
    if (strcmp(state, "high") == 0) {
        digitalWrite(pinNumber, HIGH);
        OUT("Pin %s set to HIGH", pin);
    } else if (strcmp(state, "low") == 0) {
        digitalWrite(pinNumber, LOW);
        OUT("Pin %s set to LOW", pin);
    } else {
        OUT("Invalid state: %s", state);
    }
}

void MotorApp::cmdSleep() {
    if (_motorCount == 0) { OUT("No motor configured"); return; }
    _motors[selectedMotor]->disable();
    OUT("[%s] disabled", _motors[selectedMotor]->getType());
}

void MotorApp::cmdSelectMotor() {
    int n = parser.getInt(1);
    if (_motorCount == 0) { OUT("No motor configured"); return; }
    if (n < 0 || n >= _motorCount) {
        OUT("Invalid motor index %d (valid: 0..%d)", n, _motorCount - 1);
        for (int i = 0; i < _motorCount; i++)
            OUT("  [%d] %s", i, _motors[i]->getType());
        return;
    }
    selectedMotor = n;
    OUT("Selected motor %d: %s", selectedMotor, _motors[selectedMotor]->getType());
}

void MotorApp::cmdGetServoInfo() {
    OUT("Getting servo info for ID %d", servoId);
    int pos = ST3215Motor.ReadPos(servoId);
    int speed = ST3215Motor.ReadSpeed(servoId);
    int load = ST3215Motor.ReadLoad(servoId);
    int voltage = ST3215Motor.ReadVoltage(servoId);
    int temp = ST3215Motor.ReadTemper(servoId);
    int current = ST3215Motor.ReadCurrent(servoId);

    OUT("Pos: %d  Speed: %d  Load: %d  Volt: %fV  Temp: %d°C  Current: %d mA",
        pos, speed, load, voltage / 10.0, temp, current);
}

void MotorApp::cmdPingServos() {
    OUT("Ping su servo ID %d", servoId);
    int result = ST3215Motor.Ping(servoId);
    if (result == servoId) {
        OUT("Ping: il servo con ID %d ha risposto", servoId);
    } else if (result == -1) {
        OUT("Ping: nessun servo ha risposto all'ID %d", servoId);
    } else {
        OUT("Ping: risposta inattesa da ID %d", result);
    }
}

void MotorApp::cmdScan() {
    long bauds[] = {1000000, 500000, 250000, 128000, 115200, 76800, 57600, 38400};
    int nBauds = sizeof(bauds) / sizeof(bauds[0]);
    for (int i = 0; i < nBauds; i++) {
        OUT("Setting baudrate to %d", bauds[i]);
        ST3215Motor.pSerial->begin(bauds[i], SERIAL_8N1, BUS_SERIAL_RX, BUS_SERIAL_TX);
        delay(100);
        int r = ST3215Motor.Ping(0xFE);
        if (r != -1) {
            OUT("Servo found at index %d, baudrate %d, id %d", i, bauds[i], r);
            break;
        }
    }
    OUT("Scan completed");
}

// Sblocca EEPROM, cambia ID a newId
void MotorApp::cmdSetId() {
    int newId = parser.getInt(1);
    if (newId < 0 || newId > 253) {
        OUT("ID non valido: %d (deve essere tra 0 e 253)", newId);
        return;
    }
    OUT("Sblocco EEPROM e cambio ID a %d (ID attuale=%d)", newId, servoId);
    // Sblocca EEPROM
    int res_unlock = ST3215Motor.unLockEprom(servoId);
    if (res_unlock == 1) {
        if (ST3215Motor.getError() == 0) {
            OUT("EEPROM sbloccata con successo");
        } else {
            OUT("Errore sconosciuto durante lo sblocco EEPROM, codice: %d", ST3215Motor.getError());
            return;
        }
    } else {
        OUT("Errore durante lo sblocco EEPROM");
        return;
    }

    // Scrive il nuovo ID
    int res_write = ST3215Motor.writeByte(servoId, SMS_STS_ID, newId);
    if (res_write == 1) {
        if (ST3215Motor.getError() == 0) {
            OUT("ID cambiato con successo a %d", newId);
        } else {
            OUT("Errore sconosciuto durante la scrittura dell'ID, codice: %d", ST3215Motor.getError());
            // Blocca comunque l'EEPROM per sicurezza
            ST3215Motor.LockEprom(newId);
            return;
        }
    } else {
        OUT("Errore durante la scrittura dell'ID");
        // Blocca comunque l'EEPROM per sicurezza
        ST3215Motor.LockEprom(newId);
        return;
    }

    // Blocca di nuovo l'EEPROM
    int res_lock = ST3215Motor.LockEprom(newId);
    if (res_lock == 1) {
        if (ST3215Motor.getError() == 0) {
            OUT("EEPROM bloccata di nuovo");
        } else {
            OUT("Errore sconosciuto durante il blocco EEPROM, codice: %d", ST3215Motor.getError());
        }
    } else {
        OUT("Errore nel blocco dell'EEPROM");
    }
}

// Cambia solo baudrate
void MotorApp::cmdSetBaud() {
    long baudrates[] = {1000000, 500000, 250000, 128000, 115200, 76800, 57600, 38400};
    int nBauds = sizeof(baudrates) / sizeof(baudrates[0]);
    int baudIndex = parser.getInt(1);
    if (baudIndex < 0 || baudIndex >= nBauds) {
        OUT("Indice baudrate non valido: %d", baudIndex);
        return;
    }
    uint8_t baudValue = baudIndex;
    OUT("Sblocco EEPROM per cambio baudrate (ID=%d)", servoId);
    int res_unlock = ST3215Motor.unLockEprom(servoId);
    if (res_unlock == 1) {
        if (ST3215Motor.getError() == 0) {
            OUT("EEPROM sbloccata con successo");
        } else {
            OUT("Errore sconosciuto durante lo sblocco EEPROM, codice: %d", ST3215Motor.getError());
            return;
        }
    } else {
        OUT("Errore durante lo sblocco EEPROM");
        return;
    }

    OUT("Cambio baudrate a %ld bps (ID=%d, N=%d)", baudrates[baudIndex], servoId, baudValue);
    int res_baud = ST3215Motor.writeByte(servoId, SMS_STS_BAUD_RATE, baudValue);
    if (res_baud == 1) {
        if (ST3215Motor.getError() == 0) {
            OUT("Baudrate cambiato con successo a %d bps (N=%d)", baudrates[baudIndex], baudValue);
        } else {
            OUT("Errore sconosciuto durante il cambio baudrate, codice: %d", ST3215Motor.getError());
            // Blocca comunque l'EEPROM per sicurezza
            ST3215Motor.LockEprom(servoId);
            return;
        }
    } else {
        OUT("Errore durante la scrittura del baudrate");
        // Blocca comunque l'EEPROM per sicurezza
        ST3215Motor.LockEprom(servoId);
        return;
    }

    OUT("Riavvio la seriale");
    ST3215Motor.pSerial->begin(baudrates[baudIndex], SERIAL_8N1, BUS_SERIAL_RX, BUS_SERIAL_TX);
    servoBaudrate = baudrates[baudIndex];
    delay(100);
    int ping = ST3215Motor.Ping(servoId);
    if (ping == servoId) {
        OUT("Servo risponde con ID %d e baudrate %d", servoId, servoBaudrate);
    } else {
        OUT("Servo NON risponde dopo cambio baudrate");
    }

    // Blocca di nuovo l'EEPROM
    int res_lock = ST3215Motor.LockEprom(servoId);
    if (res_lock == 1) {
        if (ST3215Motor.getError() == 0) {
            OUT("EEPROM bloccata di nuovo");
        } else {
            OUT("Errore sconosciuto durante il blocco EEPROM, codice: %d", ST3215Motor.getError());
        }
    } else {
        OUT("Errore nel blocco dell'EEPROM");
    }
}

// Imposta l'id di default da usare nei comandi
void MotorApp::cmdUseId() {
    int newId = parser.getInt(1);
    if (newId < 0 || newId > 253) {
        OUT("ID non valido: %d (deve essere tra 0 e 253)", newId);
        return;
    }
    servoId = newId;
    OUT("Impostato servoId di default a %d", servoId);
}

// Imposta il baudrate di default da usare nei comandi
void MotorApp::cmdUseRate() {
    int baudIndex = parser.getInt(1);
    long baudrates[] = {1000000, 500000, 250000, 128000, 115200, 76800, 57600, 38400};
    int nBauds = sizeof(baudrates) / sizeof(baudrates[0]);
    if (baudIndex < 0 || baudIndex >= nBauds) {
        OUT("Indice baudrate non valido: %d (range 0-%d)", baudIndex, nBauds - 1);
        return;
    }
    servoBaudrate = baudrates[baudIndex];
    ST3215Motor.pSerial->begin(servoBaudrate, SERIAL_8N1, BUS_SERIAL_RX, BUS_SERIAL_TX);
    OUT("Impostato servoBaudrate di default a %d (indice %d)", servoBaudrate, baudIndex);
}

void MotorApp::cmdFeedback() {
    if (_motorCount == 0) { OUT("No motor configured"); return; }
    enableit::Motor* m = _motors[selectedMotor];
    enableit::MotorTelemetry t = m->getTelemetry();

    // State label
    const char* stateStr = "?";
    switch (m->getState()) {
        case enableit::MotorState::UNKNOWN:    stateStr = "UNKNOWN";    break;
        case enableit::MotorState::IDLE:       stateStr = "IDLE";       break;
        case enableit::MotorState::RUNNING:    stateStr = "RUNNING";    break;
        case enableit::MotorState::MOVING:     stateStr = "MOVING";     break;
        case enableit::MotorState::HOLDING:    stateStr = "HOLDING";    break;
        case enableit::MotorState::BRAKING:    stateStr = "BRAKING";    break;
        case enableit::MotorState::CALIBRATING:stateStr = "CALIBRATING";break;
        case enableit::MotorState::HOMING:     stateStr = "HOMING";     break;
        case enableit::MotorState::SLEEPING:   stateStr = "SLEEPING";   break;
        case enableit::MotorState::FAULT:      stateStr = "FAULT";      break;
        case enableit::MotorState::MOTOR_OFF:  stateStr = "MOTOR_OFF";  break;
    }
    OUT("--- Motor %d [%s] ---", selectedMotor, m->getType());
    OUT("State    : %s", stateStr);
    OUT("Position : %d  (target: %d)", t.position, t.targetPosition);
    OUT("Speed    : %d  Velocity: %d", t.speed, t.velocity);
    OUT("Current  : %d mA", t.current_mA);
    if (t.voltage_mV    >= 0) OUT("Voltage  : %.2f V",  t.voltage_mV    / 1000.0f);
    if (t.temperature_c >= 0) OUT("Temp     : %d deg C", t.temperature_c);
    OUT("Moving: %s  Homed: %s  Enabled: %s  Fault: %s",
        t.moving?"Y":"N", t.homed?"Y":"N", t.enabled?"Y":"N", t.fault?"Y":"N");
    if (t.fault) OUT("FaultCode: %u", t.faultCode);
}

void MotorApp::cmdTestMove() {
    OUT("TestMode: muovo il servo avanti e indietro (ID=%d)", servoId);
    for (int i = 0; i < 3; ++i) { // Esegui avanti/indietro 3 volte per evitare loop infinito
        // Avanti
        ST3215Motor.RegWritePosEx(servoId, 4095, 3400, 50);
        // Se vuoi anche il secondo servo, decommenta la riga sotto:
        // ST3215Motor.RegWritePosEx(2, 4095, 3400, 50);
        ST3215Motor.RegWriteAction();
        OUT("Servo %d -> 4095", servoId);
        delay(3000);

        // Indietro
        ST3215Motor.RegWritePosEx(servoId, 0, 3400, 50);
        // Se vuoi anche il secondo servo, decommenta la riga sotto:
        // ST3215Motor.RegWritePosEx(2, 0, 3400, 50);
        ST3215Motor.RegWriteAction();
        OUT("Servo %d -> 0", servoId);
        delay(3000);
    }
    OUT("TestMode completato");
}

void MotorApp::cmdTestSync() {
    OUT("TestSync: muovo due servi in modo sincrono avanti e indietro (ID=1,2)");
    byte ID[2] = {1, 2};
    s16 Position[2];
    u16 Speed[2] = {3400, 3400};
    byte ACC[2] = {50, 50};

    // Avanti
    Position[0] = 3000;
    Position[1] = 3000;
    ST3215Motor.SyncWritePosEx(ID, 2, Position, Speed, ACC);
    OUT("Entrambi i servi -> 3000");
    delay(2000);

    // Indietro
    Position[0] = 100;
    Position[1] = 100;
    ST3215Motor.SyncWritePosEx(ID, 2, Position, Speed, ACC);
    OUT("Entrambi i servi -> 100");
    delay(2000);
}

void MotorApp::cmdOta() {
    OUT("Switching to OTA update app");
    changeApp(APP_OTAUPDATE);
}

void MotorApp::cmdBoot() {
    OUT("Switching to bootloader");
    changeApp(APP_BOOT);
}

void MotorApp::cmdReboot() {
    OUT("Rebooting board");
    changeApp(STATE_REBOOT);
}
