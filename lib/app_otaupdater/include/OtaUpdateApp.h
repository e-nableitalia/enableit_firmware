//
// OtaUpdate App definition, download and install new firmware from remote server
//
// Author: A.Navatta / e-Nable Italia

#ifndef OTAUPDATE_APP_H

#define OTAUPDATE_APP_H

#include <Arduino.h>
#include <BoardApp.h>

class OTAUpdateApp : public BoardApp {
    void enter();
    void leave();
    void process();
    const char *name();
};

#endif // OTAUPDATE_APP_H