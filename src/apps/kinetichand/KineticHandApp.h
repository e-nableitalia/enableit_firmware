//
// Pressure App, simulate a mouse controlled by pressure sensor MD30-60
//
// Author: A.Navatta / e-Nable Italia

#ifndef KINETICHAND_APP_H

#define KINETICHAND_APP_H

#include <Arduino.h>
#include <BoardApp.h>

#define APP_KINETICHAND "kinetichand"

class KineticHandApp : public BoardApp {
    void enter();
    void leave();
    void process();
    const char *name();
};

#endif // KINETICHAND_APP_H