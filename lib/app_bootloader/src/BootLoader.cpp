#include <enableit.h>

#include <Board.h>
#include <Config.h>
#include <RuntimeManager.h>
#include <SystemInfoProvider.h>

#include "BootLoader.h"

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
}

void BootLoader::init(enableit::BoardApp *s) {
    log_d("Initializing BootLoader");
    state = s;
    bootState = BootState::WAIT_USERINPUT;
    devMode = false;
    log_d(""); log_d("");
    OUT("eNable.it - Bionic Platform");

    Console.print("Booting");
    for (int i = 0; i < 10; i++) {
        Console.print(".");
        usleep(200000);
    }
    log_d("done");

    // Dump system info using SystemInfoProvider
    systeminfo.dump();

    display.println("eNable.it - Bionic Platform");
    display.println("Firmware Rev " FWREV);

    log_d("Initializing board config");
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

    log_d("Commands loaded");

    // Prepare BootConfig for runtime
    BootConfig bootCfg;
    bootCfg.wifiSsid = config.wifiSsid;
    bootCfg.wifiPassword = config.wifiPassword;
    bootCfg.apMode = config.apMode;
    bootCfg.insights = config.insights;
    bootCfg.insightsKey = config.insightsKey;
    bootCfg.deviceid = config.deviceid;
    bootCfg.thingsboard = config.thingsboard;
    bootCfg.devicetoken = config.devicetoken;
    bootCfg.mdnsHostname = "esp32"; // or config.mdnsHostname if available

    if (config.wifi) {
        runtime.startNormalMode(bootCfg);

#if THINGSBOARD_SUPPORT
        if (runtime.wifiOn() && !bootCfg.deviceid.isEmpty()) {
            log_d("Enabling thingsboard");
            if (!runtime.enableThingsBoard(bootCfg)) {
                log_e("Failed to connect to ThingsBoard");
                return;
            }
        }
#endif

        if (config.telnet) {
            log_d("Enabling telnet server");
            TelnetConsoleTransport::instance()->enable(true);
        }
    } else {
        log_d("Wifi disabled");
    }

    display.setCursor(0, 0);
    display.print("IP: ");
    display.print(board.wifi().getIpAddress());

    start = millis();
    log_d("Boot timeout[%d], start time[%d]", config.bootTimeout, start);
    OUT("Press button in %d seconds to activate dev application", config.bootTimeout);
    OUT("or press any key on USB Serial to activate interactive mode");
}

void BootLoader::fini() {

}

void BootLoader::waitUserTimeout() {
    unsigned long now = millis();

    if ((now - start) < (config.bootTimeout * 1000)) {
        int value = digitalRead(BUTTON_PIN);

        if (!value) {
            devMode = true;
        }
        
        if (Console.available() > 0) {
            log_d("BOOT: Boot procedure stopped");

            interactiveMode();
        }
    } else {
        log_d("Start[%d], now[%d]", start, now);
        if (devMode) {
            log_d("BOOT: Boot timeout expired, trying to activate dev application[%s]",config.devApp.c_str());
            if (!state->hasApp(config.devApp.c_str())) {
                log_d("BOOT: Dev application[%s] not found, going in interactive mode",config.devApp.c_str());
                interactiveMode();
                return;
            }
            state->changeApp(config.devApp.c_str());        
        } else {
            log_d("BOOT: Boot timeout expired, trying to activate default application[%s]",config.mainApp.c_str());
            if (!state->hasApp(config.mainApp.c_str())) {
                log_d("BOOT: Default application[%s] not found, going in interactive mode",config.mainApp.c_str());
                interactiveMode();
                return;
            }
            state->changeApp(config.mainApp.c_str());
        }
    }
}

void BootLoader::interactiveMode() {
    log_d("BOOT: Entering in interactive mode");
    parser.display();
    bootState = BootState::WAIT_COMMAND;
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

    if (!state) {
        OUT("No application state available.");
        return;
    }

    BoardApp **apps = state->apps();

    OUT("Available applications:");
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i] != nullptr) {
            OUT("* %s", apps[i]->name());
        }
    }
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
    BootConfig bootCfg;
    bootCfg.wifiSsid = config.wifiSsid;
    bootCfg.wifiPassword = config.wifiPassword;
    bootCfg.apMode = config.apMode;
    bootCfg.insights = config.insights;
    bootCfg.insightsKey = config.insightsKey;
    bootCfg.deviceid = config.deviceid;
    bootCfg.thingsboard = config.thingsboard;
    bootCfg.devicetoken = config.devicetoken;
    bootCfg.mdnsHostname = "esp32"; // or config.mdnsHostname if available

    if (!enabled.compareTo("true")) {
        runtime.enableWifi(bootCfg);
    } else {
        runtime.disableWifi();
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