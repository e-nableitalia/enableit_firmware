#pragma once
#include "Feature.h"
#include "Settings.h"
#include "BleCommandDispatcher.h"
#include <Arduino.h>

#define CONFIG_CHARACTERISTIC_UUID "68b788da-819b-4feb-b478-8d237ef29f5f"

class SettingsCommandDispatcher : public enableit::BleV1CommandDispatcher {
public:
    SettingsCommandDispatcher(Settings* settings);
    const char* name() const override { return "setting"; }
    void handle(const String& cmd, String& response) override;
private:
    Settings* settings_;
};
