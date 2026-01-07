#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Feature.h"
#include "FeatureRegistry.h"

enum ProtocolVersion { V1_LEGACY, V2_JSON };

class ProtocolProcessor {
public:
    ProtocolProcessor(FeatureRegistry& registry, const String& systemInfoJson);

    void process(const char* message, String& response);
    void processV1(const char* message, String& response);
    void processV2(JsonObject& msg, String& response);

private:
    FeatureRegistry& registry;
    StaticJsonDocument<512> systemInfo;
    StaticJsonDocument<512> jsonMsg;
    StaticJsonDocument<512> respDoc;
    ProtocolVersion detectProtocol(const char* message);
};