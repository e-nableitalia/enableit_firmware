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
    Serial.begin(115200);
    Serial.println("Hello from EnableIt Firmware!");
    delay(1000);
    log_i("EnableIt Firmware Starting...");
    Serial.println("Flashing Builtin LED 3 times...");
    for (int i = 0; i < 3; i++) {
        Serial.printf("Flashing LED Builtin #%d\n", i);
        log_i("Flashing LED Builtin #%d", i);
        delay(2000);
    }
    Serial.println("Setup complete.");
    log_i("Registering bootloader...");
    //REGISTER_BOARD_APP(BootLoaderApp);
    delay(2000);
    log_i("Initializing Board Manager...");
    ENABLEIT_BOOT(APP_BOOT);
}

void loop() {
    ENABLEIT_LOOP();
}