//
// BootLoaderApp: Class wrapping bootloader application in a BoardApp
//
// Author: A.Navatta / e-Nable Italia

#ifndef BOOTLOADER_APP_H

#define BOOTLOADER_APP_H

#include <enableit.h>
#include <BoardApp.h>

class BootLoaderApp : public enableit::BoardApp {
public:
    BootLoaderApp() {
        log_i("BootLoaderApp constructor");
    }
    virtual void enter();
    virtual void leave();
    virtual void process();
    virtual const char *name();
};

#endif // BOOTLOADER_APP_H