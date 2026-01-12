#pragma once
#include "Feature.h"
#include <SystemInfoProvider.h>
#include <Arduino.h>

class SystemConfigFeature : public Feature {
public:
    SystemConfigFeature();
    const char* name() const override { return "systemConfig"; }
    void handleV1(const String& cmd, String& response) override;
    void handleV2(const JsonObjectConst& msg, JsonObject& response) override;
};
