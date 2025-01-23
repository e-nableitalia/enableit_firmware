//
// Reboot App
// Simple app performing a countdown and then rebooting the board
//
// Author: A.Navatta / e-Nable Italia

#ifndef REBOOTSTATE_H

#define REBOOTSTATE_H

#include <Arduino.h>
#include <Console.h>
#include <BoardApp.h>

// Timeout in secs before reboot
#define REBOOT_LOOPS  5
#define REBOOT_DELAY    1 // seconds

class RebootState : public BoardApp {
    void enter() {
        DBG("enter reboot state");
    }
    void leave() {
        DBG("leave reboot state");
    }

    void process() {
        DBG("process");
        LOG("");
        for (int i = REBOOT_LOOPS; i > 0; i--) {
            LOG("Rebooting in %d seconds", i);
            sleep(REBOOT_DELAY);
        }
        LOG("Rebooting now");
        ESP.restart();
    }

    const char *name() { return STATE_REBOOT; }
};

#endif // REBOOTSTATE_H