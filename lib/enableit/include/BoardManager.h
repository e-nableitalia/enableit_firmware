//
// BoardManager: enable.it platform/board implementation
//
// Author: A.Navatta / e-Nable Italia

#ifndef BOARD_MANAGER_H

#define BOARD_MANAGER_H

#include <Arduino.h>

// This library provides the functionality of OTA Firmware Upgrade
#include <HttpsOTAUpdate.h>
#include <Console.h>

#include <BoardApp.h>

// Timeout in secs for error print in dev mode
#define PANIC_TIMEOUT   20

// Enable to add delay to main loop
//#define LOOP_DELAY      500

class BoardManager {
public:
    BoardManager();
    void init();
    void loop();

    bool setApp(const char *state_name);
    bool addApp(BoardApp *state);

    void panic(int code, const char *description);

    BoardApp **getApps();
    BoardApp *getCurrentApp();

private:
    void setApp(BoardApp *state);

    BoardApp *currentApp;
    BoardApp *apps[MAX_APPS];
};

#if !defined(NO_GLOBAL_INSTANCES)
extern BoardManager eBoard;
#endif

#endif // BOARD_MANAGER_H