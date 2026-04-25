#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Enum for feature protocol version
enum class FeatureVersion {
    V1,
    V2
};

// Abstract base interface for protocol/domain features (hand, settings, emg, etc)
class FeatureBase {
public:
    virtual ~FeatureBase() {}
    // Returns the feature name (e.g., "hand", "settings", "emg")
    virtual const char* name() const = 0;
    // Returns the feature protocol version
    virtual FeatureVersion version() const = 0;
};

// Abstract interface for v1 features (plain text)
class FeatureV1 : public FeatureBase {
public:
    virtual ~FeatureV1() {}
    // Handles legacy v1 command (plain text)
    virtual void handle(const String& cmd, String& response) = 0;
    // Returns the feature protocol version
    inline FeatureVersion version() const override { return FeatureVersion::V1; }
};

// Abstract interface for v2 features (JSON)
class FeatureV2 : public FeatureBase {
public:
    virtual ~FeatureV2() {}
    // Handles v2 command (JSON)
    virtual void handle(const JsonObjectConst& msg, JsonObject& response) = 0;
    // Returns the feature protocol version
    inline FeatureVersion version() const override { return FeatureVersion::V2; }
};
