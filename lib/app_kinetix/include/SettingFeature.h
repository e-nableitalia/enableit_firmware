#pragma once
#include "Feature.h"
#include "Settings.h"
#include <Arduino.h>

class SettingFeature : public Feature {
public:
    SettingFeature(Settings* settings);
    const char* name() const override { return "setting"; }
    void handleV1(const String& cmd, String& response) override;
    void handleV2(const JsonObjectConst& msg, JsonObject& response) override;
private:
    Settings* settings_;
};
