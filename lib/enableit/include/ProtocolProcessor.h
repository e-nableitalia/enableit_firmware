#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Feature.h"
#include "FeatureRegistry.h"

enum ProtocolVersion { V1_LEGACY, V2_JSON };

class ProtocolProcessor {
public:
    ProtocolProcessor(FeatureRegistry& registry);

    void process(const char* message, String& response);
    void processV1(const char* message, String& response);
    void processV2(JsonObject& msg, String& response);

private:
    FeatureRegistry& registry;
    JsonDocument jsonMsg;
    JsonDocument respDoc;
    ProtocolVersion detectProtocol(const char* message);
};