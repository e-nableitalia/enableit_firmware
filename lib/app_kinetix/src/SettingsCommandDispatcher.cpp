#include "SettingsCommandDispatcher.h"
#include <ArduinoJson.h>

#include <Arduino.h>

SettingsCommandDispatcher::SettingsCommandDispatcher(Settings* settings)
    : BleV1CommandDispatcher( CONFIG_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ ),
      settings_(settings) {
    Serial.println("[SettingsCommandDispatcher] Initialized");
}

void SettingsCommandDispatcher::handle(const String& cmd, String& response) {
    Serial.print("[SettingsCommandDispatcher] handle called with cmd: ");
    Serial.println(cmd);

    // V1: setting:<json> (write), setting (read)
    if (cmd.length() == 0) {
        // Read
        Serial.println("[SettingsCommandDispatcher] Reading settings");
        response = settings_->getSettingJson();
    } else {
        // Write
        Serial.print("[SettingsCommandDispatcher] Updating settings with: ");
        Serial.println(cmd);
        settings_->updateSetting(cmd.c_str());
        response = "OK: setting updated";
        Serial.println("[SettingsCommandDispatcher] Settings updated");
    }
}