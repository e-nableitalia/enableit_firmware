//
// Web Update App
// Exposes a web server for firmware update
//
// Author: A.Navatta / e-Nable Italia

#ifndef OTAWEBUPDATER_APP_H

#define OTAWEBUPDATER_APP_H

#include <Arduino.h>
#include <Console.h>
#include <BoardApp.h>

#define APP_OTAWEBUPDATER "otaweb"

using namespace enableit;

class OTAWebUpdater : public BoardApp {
    void enter();
    void leave() {
        log_d("leave OTA Web Updater state");
    }

    void process();
    const char *name() { return APP_OTAWEBUPDATER; }
};

#endif // OTAWEBUPDATER_APP_H