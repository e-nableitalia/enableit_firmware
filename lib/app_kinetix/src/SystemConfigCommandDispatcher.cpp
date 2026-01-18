#include "SystemConfigCommandDispatcher.h"
#include <SystemInfoProvider.h>
#include <ArduinoJson.h>

#include <Arduino.h>

SystemConfigCommandDispatcher::SystemConfigCommandDispatcher() : BleV1CommandDispatcher( SYSTEM_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ ) {
    Serial.println("[SystemConfigCommandDispatcher] Constructor called");
}

void SystemConfigCommandDispatcher::handle(const String& cmd, String& response) {
    Serial.print("[SystemConfigCommandDispatcher] Handling command: ");
    Serial.println(cmd);

    // V1: systemConfig (read-only)
    enableit::systeminfo.serialize(response);

    Serial.print("[SystemConfigCommandDispatcher] Response: ");
    Serial.println(response);
}
