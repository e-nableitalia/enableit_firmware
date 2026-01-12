#include "SystemConfigFeature.h"
#include <SystemInfoProvider.h>
#include <ArduinoJson.h>

SystemConfigFeature::SystemConfigFeature() {}

void SystemConfigFeature::handleV1(const String& cmd, String& response) {
    // V1: systemConfig (read-only)
    enableit::systeminfo.serialize(response);
}

void SystemConfigFeature::handleV2(const JsonObjectConst& msg, JsonObject& response) {
    // V2: { target: "systemConfig", action: "get" }
    log_i("Handling V2 systemConfig command");
    response["status"] = "error";
    response["error"] = "systemConfig feature is read-only and only supports V1 commands";
}
