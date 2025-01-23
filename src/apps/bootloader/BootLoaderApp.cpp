#include <Arduino.h>
#include <Console.h>
#include "BootLoaderApp.h"
#include "BootLoader.h"

BootLoader bootloader;

void BootLoaderApp::enter() {
    DBG("Entering in Boot Loader state");
    bootloader.init(this);
}
void BootLoaderApp::leave() {
    DBG("Leaving boot Loader state");
    bootloader.fini();
}

void BootLoaderApp::process() {
    //DBG("Called process in BootLoader state");
    bootloader.run();
}

const char *BootLoaderApp::name() {
    return APP_BOOT;
}
