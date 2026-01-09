//
// main.cpp
//
// initialize eBoard, add running states (apps) and activate bootloader
//
// Author: A.Navatta / e-Nable Italia

#include <Arduino.h>
#include <BoardManager.h>

//DECLARE_BOARDAPP(BootLoaderApp);
ENABLE_BOARD_APP(BootLoaderApp);

void setup() {
    ENABLEIT_BOOT(APP_BOOT);
}

void loop() {
    ENABLEIT_LOOP();
}