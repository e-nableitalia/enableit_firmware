//
// NoopApp class definition, dummy app for tests
//
// Author: A.Navatta / e-Nable Italia

#ifndef NOOP_APP_H

#define NOOP_APP_H

#include <Arduino.h>
#include <Console.h>
#include <BoardApp.h>

#include "Insights.h"
#include "esp_err.h"

#define TAG "NoopApp"

class NoopApp : public BoardApp {
    void enter() {
        DBG("enter");
        Insights.event(TAG, "Entering");
        log_i("Entering in Noop state");
    }
    void leave() {
        DBG("leave");
        Insights.event(TAG, "Leaving");
        log_e("Leaving Noop state");
    }

    void process() {
        DBG("process");
        log_d("Called process in Noop state, sleep 1000 ms");
        Insights.event(TAG, "Processing...");
        delay(1000);
    }

    const char *name() { return APP_NOOP; }
};

#endif // NOOP_APP_H