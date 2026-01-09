#include <Arduino.h>
#include <Console.h>
#include "BootLoaderApp.h"
#include "BootLoader.h"

static BootLoader bootloader;

void BootLoaderApp::enter() {
    log_i("Entering in Boot Loader state");
    bootloader.init(this);
}
void BootLoaderApp::leave() {
    log_i("Leaving boot Loader state");
    bootloader.fini();
}

void BootLoaderApp::process() {
    //log_d("Called process in BootLoader state");
    bootloader.run();
}

const char *BootLoaderApp::name() {
    return APP_BOOT;
}

BOARDAPP_INSTANCE(BootLoaderApp);

// BootLoaderApp bootloaderApp;

// REGISTER_BOARD_APP(BootLoaderApp, bootloaderApp);
