#include "SettingFeature.h"
#include <ArduinoJson.h>

SettingFeature::SettingFeature(Settings* settings)
    : settings_(settings) {}

void SettingFeature::handleV1(const String& cmd, String& response) {
    // V1: setting:<json> (write), setting (read)
    if (cmd.length() == 0) {
        // Read
        response = settings_->getSettingJson();
    } else {
        // Write
        settings_->updateSetting(cmd.c_str());
        response = "OK: setting updated";
    }
}

void SettingFeature::handleV2(const JsonObjectConst& msg, JsonObject& response) {
    // V2: { target: "setting", action: "get"|"set", value: <json> }
    if (!msg.containsKey("action")) {
        response["status"] = "error";
        response["error"] = "Missing action";
        return;
    }
    String action = msg["action"].as<String>();
    if (action == "get") {
        response["status"] = "ok";
        response["value"] = settings_->getSettingJson();
    } else if (action == "set" && msg.containsKey("value")) {
        settings_->updateSetting(msg["value"].as<const char*>());
        response["status"] = "ok";
        response["result"] = "setting updated";
    } else {
        response["status"] = "error";
        response["error"] = "Unknown or missing action/value";
    }
}
