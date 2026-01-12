//
// Kinetix App definition, download and install new firmware from remote server
//
// Author: A.Navatta / e-Nable Italia

#pragma once
#include <Arduino.h>
#include <BoardApp.h>
#include "MovementFeature.h"
#include "OptionalSensorProcessor.h"

class KinetixApp : public enableit::BoardApp {
    void enter();
    void leave();
    void process();
    const char *name();
    private:
    MovementFeature *movementFeature_ = nullptr;
    Settings settings_;
    RealSensorProcessor *sensorProcessor_ = nullptr;
};
