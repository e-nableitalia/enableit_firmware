//
// main.cpp
//
// initialize eBoard, add running states (apps) and activate bootloader
//
// Author: A.Navatta / e-Nable Italia

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>

#include <BoardManager.h>
#include <NoopApp.h>
#include <RebootApp.h>

#include "apps/bootloader/BootLoaderApp.h"
#include "apps/otaupdater/OtaUpdateApp.h"
#include "apps/webupdater/OTAWebUpdaterApp.h"
#include "apps/emgdemo/EMGApp.h"
#include "apps/pressuremouse/PressureApp.h"
#include "apps/kinetichand/KineticHandApp.h"
#include "apps/WifiTest/WifiTest.h"
#include "apps/motor/MotorApp.h"
#include "apps/board/BoardTest.h"

BootLoaderApp boot;
NoopApp       noop;
OTAUpdateApp  otaupdate;
OTAWebUpdater otaweb;
RebootState   reboot;
EMGApp        emg;
PressureApp   pressure;
KineticHandApp kinetic;
WifiApp       wifiapp;
MotorApp      motor;
BoardTest     boardTest;

//#define TEST_MODE

void setup() {
#ifndef TEST_MODE
    // initialize eBoar
    eBoard.init();
    
    // add running states/applications
    eBoard.addApp(&boot);
    eBoard.addApp(&noop);
    eBoard.addApp(&otaupdate);
    eBoard.addApp(&otaweb);
    eBoard.addApp(&reboot);
    eBoard.addApp(&emg);
    eBoard.addApp(&pressure);
    eBoard.addApp(&kinetic);
    eBoard.addApp(&wifiapp);
    eBoard.addApp(&motor);
    eBoard.addApp(&boardTest);

    // activate default state: bootloader
    eBoard.setApp(APP_BOOT);
#else
    Serial.begin(115200);
    pinMode(8, OUTPUT);
    Serial.println("Test mode");
#endif
}

void loop() {
#ifndef TEST_MODE 
    // main loop
   eBoard.loop();
#else
    digitalWrite(8, HIGH);
    Serial.println("Pin High");
    delay(1000);
    digitalWrite(8, LOW);
    Serial.println("Pin Low");
    delay(1000);
#endif
}
