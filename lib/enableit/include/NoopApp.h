//
// NoopApp class definition, dummy app for tests
//
// Author: A.Navatta / e-Nable Italia

#pragma once

#include <Arduino.h>
#include <Console.h>
#include <BoardApp.h>

#if defined(INSIGHTS_SUPPORT)
#include "Insights.h"
#endif
#include "esp_err.h"

#define TAG "NoopApp"

using namespace enableit;

class NoopApp : public BoardApp {
    void enter() {
        log_d("enter");
#if defined(INSIGHTS_SUPPORT)        
        Insights.event(TAG, "Entering");
#endif
        log_i("Entering in Noop state");
    }
    void leave() {
        log_d("leave");
#if defined(INSIGHTS_SUPPORT)        
        Insights.event(TAG, "Leaving");
#endif
        log_e("Leaving Noop state");
    }

    void process() {
        log_d("process");
        log_d("Called process in Noop state, sleep 1000 ms");
#if defined(INSIGHTS_SUPPORT)
        Insights.event(TAG, "Processing...");
#endif
        delay(1000);
    }

    const char *name() { return APP_NOOP; }
};

