#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Abstract interface for protocol/domain features (hand, settings, emg, etc)
class Feature {
public:
    virtual ~Feature() {}

    // Returns the feature name (e.g., "hand", "settings", "emg")
    virtual const char* name() const = 0;

    // Handles legacy v1 command (plain text)
    virtual void handleV1(const String& cmd, String& response) = 0;

    // Handles v2 command (JSON)
    virtual void handleV2(const JsonObjectConst& msg, JsonObject& response) = 0;
};
