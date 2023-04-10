#include <BootLoader.h>
#include <Insights.h>
#include <Esp.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Config.h>

#define CMD_HELP    "* help - show this help"
#define CMD_INFO    "* info - show active configuration"
#define CMD_SET     "* set <parameter> <value> - set parameter <parameter> with value <value>"
#define CMD_ENABLE  "* enable <password> - enable privileged mode"
#define CMD_DISABLE "* disable - leave privileged mode"
#define CMD_REBOOT  "* reboot - reboot board"
#define CMD_BOOT    "* boot - continue boot starting board manager"
#define CMD_WIFION  "* wifion <true/false> - activate/deactivate wifi"
#define CMD_OTAUPD  "* otaupdate - execute OTA firmware update"
#define CMD_STORE   "* store - store current settings"
#define CMD_ERASE   "* erase - erase stored settings"
#define CMD_UNLOCK  "* unlock <token> - unlock privileged mode"
#define CMD_SECRET  "* secret <secretkey> - set encryption key to [secretkey]"
#define CMD_RUN     "* run <app> - execute application <app>"
#define CMD_LIST    "* list - list applications"

BootLoader::BootLoader() {
    // noop
}

void BootLoader::init(BoardApp *s) {
    DBG("Initializing BootLoader");
    state = s;
    bootState = BootState::WAIT_USERINPUT;

//    pinMode(BUTTON_PIN, INPUT);
//    pinMode(LED_PIN, OUTPUT);
    DBG("I/O pin configured");

    DBG(""); DBG("");
    OUT("eNable.it - Bionic Platform");

    Serial.print("Booting");
    for (int i = 0; i < 10; i++) {
        Serial.print(".");
        usleep(200000);
    }
    Serial.println("done");

    OUT("Firmware Rev %s", FWREV);
    
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
 
    OUT("HwInfo:");
  
    OUT("%d cores Wifi%s%s, %d Mhz", chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "", ESP.getCpuFreqMHz());
    OUT("Silicon revision: %d", chip_info.revision);
    OUT("%dMB %s flash, Memory %d", spi_flash_get_chip_size()/(1024*1024),
                (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embeded" : "external", ESP.getPsramSize());
 
    //get chip id
    String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
    chipId.toUpperCase();
 
    OUT("SDK: %s, Chip: %s, Chip id: %s", ESP.getSdkVersion(), ESP.getChipModel(), chipId.c_str());
    OUT("Fw Checksum: %s", ESP.getSketchMD5().c_str());

    DBG("Initializing board config");
    config.init();

    if (config.devApp == "") {
        config.devApp = s->name();
    }
    if (config.mainApp == "") {
        config.mainApp = "init";
    }
    OUT("Applications: default[%s], boot[%s]", config.mainApp.c_str(), config.devApp.c_str());

    parser.init(this);
    parser.add("help", CMD_HELP, &BootLoader::cmdHelp);
    parser.add("info", CMD_INFO, &BootLoader::cmdInfo);
    parser.add("set", CMD_SET, &BootLoader::cmdSet);
    parser.add("reboot", CMD_REBOOT, &BootLoader::cmdReboot);
    parser.add("boot", CMD_BOOT, &BootLoader::cmdBoot);
    parser.add("wifion", CMD_WIFION, &BootLoader::cmdWifion);
    parser.add("enable", CMD_ENABLE, &BootLoader::cmdEnable);
    parser.add("disable", CMD_DISABLE, &BootLoader::cmdDisable);
    parser.add("otaupdate", CMD_OTAUPD, &BootLoader::cmdOtaUpdate);
    parser.add("secret", CMD_SECRET, &BootLoader::cmdSetSecretKey);
    parser.add("store", CMD_STORE, &BootLoader::cmdSave);
    parser.add("unlock", CMD_UNLOCK, &BootLoader::cmdUnlock);
    parser.add("erase", CMD_ERASE, &BootLoader::cmdErase);
    parser.add("run", CMD_RUN, &BootLoader::cmdRun);
    parser.add("list", CMD_LIST, &BootLoader::cmdList);

    DBG("Commands loaded");

    if (config.wifi) {
        doEnableWifi();
    } else {
        if (config.insights) {
            ERR("ESP Insights disabled, requires WiFi active to enable it");
        }
        wifion = false;
    }

    start = millis();
    DBG("Boot timeout[%d], start time[%d]", config.bootTimeout, start);
    OUT("Press button in %d seconds to enter bootloader", config.bootTimeout);
}

void BootLoader::fini() {

}

void BootLoader::waitUserTimeout() {
    unsigned long now = millis();

    if ((now - start) < (config.bootTimeout * 1000)) {
        DBG("BOOT: Checking button state/serial in");
        int value = digitalRead(BUTTON_PIN);
        
        if ((!value) || (Console.available() > 0)) {
            DBG("BOOT: Boot procedure stopped, switching to [%s]", config.devApp.c_str());
            //digitalWrite(LED_PIN,0);
            if (config.devApp == "boot") {
                parser.display();
                bootState = BootState::WAIT_COMMAND;
            } else {
                state->changeApp(config.devApp.c_str());        
            }
        }
    } else {
        DBG("BOOT: Boot timeout expired, activating default application[%s]",config.mainApp.c_str());
        state->changeApp(config.mainApp.c_str());
    }
}

void BootLoader::doEnableWifi() {
    if (wifion) {
        ERR("WiFi already active");
        return;
    }

    DBG("Activating WIFI");
    if (config.apMode) {
#ifndef ARDUINO_BPI_LEAF_S3
        WiFi.mode(WIFI_AP);
#else
#pragma message "WIFI_AP disabled for ARDUINO_BPI_LEAF_S3"
#endif
        OUTNOLF("Activating WiFi AP SSID[%s]", config.wifiSsid.c_str());
 #ifndef  ARDUINO_BPI_LEAF_S3
        WiFi.softAP((char *)config.wifiSsid.c_str(), config.wifiPassword.c_str());
#else
#pragma message "WiFi AP Mode disabled for ARDUINO_BPI_LEAF_S3"
#endif
    } else {
#ifndef ARDUINO_BPI_LEAF_S3
        WiFi.mode(WIFI_STA);
#else
#pragma message "WIFI_STA disabled for ARDUINO_BPI_LEAF_S3"
#endif
        OUTNOLF("Wifi init, connecting to[%s]", config.wifiSsid.c_str());
        const char *ssid = config.wifiSsid.c_str();
        const char *pwd = config.wifiPassword.c_str();
        WiFi.begin(ssid, pwd);
    }

    int count = 0;

    while ((WiFi.status() != WL_CONNECTED)&& (count < MAX_WIFI_CONNECT_ATTEMPTS)) {
        delay(WIFI_CHECK_DELAY);
        OUTNOLF(".");
        count++;
    }
    LOG("");
    if ((WiFi.status() == WL_CONNECTED)) {
        LOG("WiFi connected");

        LOG("IP address: [%s]", WiFi.localIP().toString().c_str());

        if (!MDNS.begin("esp32")) { 
            ("Error setting up MDNS responder!");
        }
        LOG("mDNS responder started");

        if (config.insights) {
            if(Insights.begin(config.insightsKey.c_str())){
                DBG("=========================================");
                DBG("ESP Insights enabled Node ID %s", Insights.nodeID());
                DBG("=========================================");
            } else {
                DBG("=========================================");
                DBG("ESP Insights enable failed");
                DBG("=========================================");
            }
        }

        wifion = true;
    } else {
        ERR("Wifi init failed");
        wifion = false;

        OUT("Scanning for known wifi");
        int n = WiFi.scanNetworks();
        OUT("Found %d networks",n);
        for (int i = 0; i < n;i++) {
            OUT("SSID[%s] RSSI[%d], Encryption[%s]", WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
        }
    }
}

void BootLoader::doDisableWifi() {
    if (!wifion) {
        ERR("WiFi already disactivated");
        return;
    }

    LOG("Deativating WIFI");

    if (config.insights) {
        DBG("Stopping insights");
        Insights.end();
    }

    WiFi.disconnect();
#if defined(ARDUINO_BPI_LEAF_S3)
    DBG("WiFi disable disabled");
#else
    WiFi.mode(WIFI_OFF);
#endif

    OUT("Wifi disabled");

    wifion = false;
}    

void BootLoader::cmdHelp() {
    OUT("** Help **");
    OUT("Commands:");
    for (int i = 0; i < MAX_COMMANDS; i++) {
        const char *h = parser.getHelp(i);
        if (h) OUT(h);
    }
    OUT("");
    OUT("Set command parameters:");
    OUT("* timeout <timeout> - set boot timeout to <timeout> seconds");
    OUT("* devmode <true/false> - activate/deactivate developer mode");
    OUT("* wifisid <SSID> - set wifi SSID");
    OUT("* wifipwd <password> - set wifi password");
    OUT("* wifi <true/false> - set wifi enabled/disabled");
    OUT("* insights <true/false> - insights activated (requires wifi active & insights key set)/deactivated");
    OUT("* insightsKey <secret key> - insights secret key");
    OUT("* apmode <true/false> - enable Access Point mode");
    OUT("* otaurl <url> - set OTA Update server url");
    OUT("");
    OUT("Privileged mode[%s]", config.isPrivileged() ? "enabled" : "disabled");
}

void BootLoader::cmdRun() {
    OUT("** Run App **");

    String app = parser.getString(1);

    OUT("Executing app[%s]", app.c_str());
    state->changeApp((char *)app.c_str());
}

void BootLoader::cmdList() {
    OUT("** List Apps **");

    OUT("** TBA **");
}


void BootLoader::cmdUnlock() {
    OUT("** Unlock **");

    String pwd = parser.getString(1);

    config.unlock(pwd);
}

void BootLoader::cmdSetSecretKey() {
    OUT("** Secret **");

    String pwd = parser.getString(1);

    config.setSecretKey(pwd);
}

void BootLoader::cmdErase() {
    OUT("** Erase User Flash **");

}

void BootLoader::cmdOtaUpdate() {
    OUT("** Ota Update **");
    state->changeApp(APP_OTAUPDATE);
}

void BootLoader::cmdEnable() {
    OUT("** Enable **");

    String enablepwd = parser.getString(1);

    config.enable(enablepwd);
}

void BootLoader::cmdDisable() {
    OUT("** Disable **");

    config.disable();
}

void BootLoader::cmdSave() {
    OUT("** Save **");

    config.save();
}

void BootLoader::cmdWifion() {
    OUT("** Wifion **");
    String enabled = parser.getString(1);

    enabled.toLowerCase();
    if (!enabled.compareTo("true")) {
        doEnableWifi();
    } else {
        doDisableWifi();
    }
}

void BootLoader::cmdReboot() {
    OUT("** Reboot **");
    state->changeApp(STATE_REBOOT);
}

void BootLoader::cmdBoot() {
    OUT("** Boot **");
    state->changeApp(STATE_INIT);
}

void BootLoader::cmdInfo() {
    OUT("** Info **");
    String dump = config.dump();
    OUT(dump.c_str());
    config.fullDump();
}

void BootLoader::cmdSet() {
    OUT("** Set **");
    OUT("Parameter[%s], value[%s]", parser.getString(1), parser.getString(2));
    config.set(parser.getString(1), parser.getString(2));
}

void BootLoader::run() {
    switch (bootState) {
        case BootState::WAIT_COMMAND: {
            parser.poll();
        }
        break;
        case BootState::WAIT_COMMAND_PRI:
        break;
        case BootState::WAIT_USERINPUT:
        default:
#if 1
            // DEBUG, commented out waitUserTimeout -> going directly in WAIT_COMMAND
            waitUserTimeout();
#else
            parser.display();
            bootState = BootState::WAIT_COMMAND;
#endif
    }
}