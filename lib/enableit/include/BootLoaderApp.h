//
// BootLoaderApp: Class wrapping bootloader application in a BoardApp
//
// Author: A.Navatta / e-Nable Italia

#ifndef BOOTLOADER_APP_H

#define BOOTLOADER_APP_H

#include <Arduino.h>
#include <BoardApp.h>

class BootLoaderApp : public BoardApp {
    void enter();
    void leave();
    void process();
    const char *name();
};

#endif // BOOTLOADER_APP_H