#include "SettingsCommandDispatcher.h"
#include <ArduinoJson.h>

#include <enableit.h>

SettingsCommandDispatcher::SettingsCommandDispatcher(Settings* settings)
    : BleV1CommandDispatcher( CONFIG_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ ),
      settings_(settings) {
    log_d("[SettingsCommandDispatcher] Initialized");
}

void SettingsCommandDispatcher::handle(const String& cmd, String& response) {
    log_d("[SettingsCommandDispatcher] handle called with cmd: %s", cmd.c_str());

    // V1: setting:<json> (write), setting (read)
    if (cmd.length() == 0) {
        // Read
        log_d("[SettingsCommandDispatcher] Reading settings");
        response = settings_->getSettingJson();
        log_d("[SettingsCommandDispatcher] Current settings: %s", response.c_str());
    } else {
        // Write
        log_d("[SettingsCommandDispatcher] Updating settings with: %s", cmd.c_str());
        settings_->updateSetting(cmd.c_str());
        //response = "OK: setting updated";
        log_d("[SettingsCommandDispatcher] Settings updated");
    }
}
