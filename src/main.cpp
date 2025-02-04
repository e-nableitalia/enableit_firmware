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

BootLoaderApp boot;
NoopApp       noop;
OTAUpdateApp  otaupdate;
OTAWebUpdater otaweb;
RebootState   reboot;
EMGApp        emg;
PressureApp   pressure;
KineticHandApp kinetic;
WifiApp       wifiapp;

void setup() {
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

    // activate default state: bootloader
    eBoard.setApp(APP_BOOT);
}

void loop() {
    // main loop
   eBoard.loop();
}