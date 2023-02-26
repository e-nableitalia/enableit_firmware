//
// Web Update App
// Exposes a web server for firmware update
//
// Author: A.Navatta / e-Nable Italia

#ifndef OTAWEBUPDATER_APP_H

#define OTAWEBUPDATER_APP_H

#include <Arduino.h>
#include <debug.h>
#include <BoardApp.h>

#define STATE_OTAWEBUPDATER "otaweb"

class OTAWebUpdater : public BoardApp {
    void enter();
    void leave() {
        DBG("leave OTA Web Updater state");
    }

    void process();
    const char *name() { return STATE_OTAWEBUPDATER; }
};

#endif // OTAWEBUPDATER_APP_H