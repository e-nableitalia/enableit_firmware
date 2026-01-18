//
// main.cpp
//
// initialize eBoard, add running states (apps) and activate bootloader
//
// Author: A.Navatta / e-Nable Italia

#include <Arduino.h>
#include <BoardManager.h>

ENABLE_BOARD_APP(DemoApp);
ENABLE_BOARD_APP(KinetixApp);
ENABLE_BOARD_APP(BootLoaderApp);

void setup() {
    ENABLEIT_BOOT("kinetix");
}

void loop() {
    ENABLEIT_LOOP();
}