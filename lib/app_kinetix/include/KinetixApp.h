//
// Kinetix App definition
//
// reafactoring of original KinetixApp from reivaxy / Kinetix Firmware
// github repo: https://github.com/reivaxy/kinetix
// updated to original git repo commitid: d2da4e554a442f09d542de8424e25a61b0356f79
//
// Author: A.Navatta / e-Nable Italia


#pragma once
#include <enableit.h>
#include <BoardApp.h>
#include "MovementCommandDispatcher.h"
#include "OptionalSensorProcessor.h"

class KinetixApp : public enableit::BoardApp {
    void enter();
    void leave();
    void process();
    const char *name();
private:
    MovementCommandDispatcher *movementCommandDispatcher_ = nullptr;
    Settings settings_;
    SensorProcessor *sensorProcessor_ = nullptr;
    bool lastBleConnected = false;
};
