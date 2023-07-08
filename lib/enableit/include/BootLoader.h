//
// bootloader: Boot loader application, handles:
// * board initial config
// * save/load of config in flash
// * wifi configuration (Station or Access Point)
// * boot procedure & console config application
//
// Author: A.Navatta / e-Nable Italia

#ifndef BOOTLOADER_H

#define BOOTLOADER_H

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ThingsBoard.h>
#include <debug.h>
#include <BoardApp.h>

#include <CommandParser.h>

#define FWREV    "1.0." __DATE__ "." __TIME__

#define BUTTON_PIN  GPIO_NUM_0
//#define LED_PIN     GPIO_NUM_25
#define WIFI_CHECK_DELAY            500 // 500ms delay
#define MAX_WIFI_CONNECT_ATTEMPTS   20 // 10 attempts * 500ms => 5 seconds

class BootLoader {
public:
    BootLoader();

    enum BootState {
        WAIT_USERINPUT,
        WAIT_COMMAND,
        WAIT_COMMAND_PRI
    } bootState;

    void init(BoardApp *state);
    void fini();

    void run();
private:
    void waitUserTimeout();

    void cmdBoot();
    void cmdReboot();
    void cmdHelp();
    void cmdInfo();
    void cmdSet();
    void cmdWifion();
    void cmdOtaUpdate();
    void cmdEnable();
    void cmdDisable();
    void cmdSave();
    void cmdSetSecretKey();
    void cmdErase();
    void cmdUnlock();
    void cmdRun();
    void cmdList();

    void doEnableWifi();
    void doDisableWifi();

    unsigned long start;
    BoardApp *state;

    CommandParser<BootLoader> parser;
    bool wifion;
};

#endif // BOOTLOADER_H