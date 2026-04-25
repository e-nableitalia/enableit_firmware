//
// Pressure App, simulate a mouse controlled by pressure sensor MD30-60
//
// Author: A.Navatta / e-Nable Italia

#ifndef PRESSURE_APP_H

#define PRESSURE_APP_H

#include <enableit.h>
#include <BoardApp.h>

#define APP_PRESSURE "pressure"

using namespace enableit;

class PressureApp : public BoardApp {
    void enter();
    void leave();
    void process();
    const char *name();
};

#endif // OTAUPDATE_APP_H