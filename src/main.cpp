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

#include <BoardManager.h>
#include <NoopApp.h>
#include <BootloaderApp.h>
#include <OTAUpdateApp.h>
#include <OTAWebUpdaterApp.h>
#include <RebootApp.h>
#include <Preferences.h>

BootLoaderApp boot;
NoopApp       noop;
OTAUpdateApp  otaupdate;
OTAWebUpdater otaweb;
RebootState   reboot;

void setup() {
    // initialize eBoar
    eBoard.init();
    
    // add running states/applications
    eBoard.addApp(&boot);
    eBoard.addApp(&noop);
    eBoard.addApp(&otaupdate);
    eBoard.addApp(&otaweb);
    eBoard.addApp(&reboot);

    // activate default state: bootloader
    eBoard.setApp(APP_BOOT);
}

void loop() {
    // main loop
   eBoard.loop();
}