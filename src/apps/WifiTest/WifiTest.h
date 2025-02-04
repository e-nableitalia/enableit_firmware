//
// Reboot App
// Simple app performing a countdown and then rebooting the board
//
// Author: A.Navatta / e-Nable Italia

#ifndef WIFIT_APP_H

#define WIFIT_APP_H

#include <Arduino.h>
#include <Console.h>
#include <BoardApp.h>

#define APP_WIFITEST "wifi"

class WifiApp : public BoardApp {
    void enter();
    void leave();
    void process();
    const char *name();
};

#endif // REBOOTSTATE_H