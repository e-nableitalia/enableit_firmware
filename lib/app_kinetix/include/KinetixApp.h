//
// Kinetix App definition, download and install new firmware from remote server
//
// Author: A.Navatta / e-Nable Italia

#pragma once
#include <Arduino.h>
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
};
