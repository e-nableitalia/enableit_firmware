//
// Reboot App
// Simple app performing a countdown and then rebooting the board
//
// Author: A.Navatta / e-Nable Italia

#pragma once

#include <Arduino.h>
#include <Console.h>
#include <BoardApp.h>

// Timeout in secs before reboot
#define REBOOT_LOOPS  5
#define REBOOT_DELAY    1 // seconds

using namespace enableit;

class RebootState : public BoardApp {
    void enter() {
        log_d("enter reboot state");
    }
    void leave() {
        log_d("leave reboot state");
    }

    void process() {
        log_d("process");
        log_i("");
        for (int i = REBOOT_LOOPS; i > 0; i--) {
            log_i("Rebooting in %d seconds", i);
            sleep(REBOOT_DELAY);
        }
        log_i("Rebooting now");
        ESP.restart();
    }

    const char *name() { return STATE_REBOOT; }
};


