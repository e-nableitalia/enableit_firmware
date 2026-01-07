//
// main.cpp
//
// initialize eBoard, add running states (apps) and activate bootloader
//
// Author: A.Navatta / e-Nable Italia

#include <Arduino.h>
#include <BoardManager.h>

void setup() {
    ENABLEIT_BOOT();
}

void loop() {
    ENABLEIT_LOOP();
}