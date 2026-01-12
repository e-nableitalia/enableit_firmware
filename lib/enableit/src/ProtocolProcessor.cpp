#include "ProtocolProcessor.h"

// Rename BLEMessageProcessor to ProtocolProcessor
ProtocolProcessor::ProtocolProcessor(FeatureRegistry& registry)
    : registry(registry)
{
}

ProtocolVersion ProtocolProcessor::detectProtocol(const char* message) {
    if (message && message[0] == '{') {
        return V2_JSON;
    }
    return V1_LEGACY;
}

void ProtocolProcessor::process(const char* message, String& response) {
    ProtocolVersion proto = detectProtocol(message);
    if (proto == V2_JSON) {
        jsonMsg.clear();
        DeserializationError err = deserializeJson(jsonMsg, message);
        if (err) {
            respDoc.clear();
            respDoc["status"] = "error";
            respDoc["error"] = String("JSON parse error: ") + err.c_str();
            serializeJson(respDoc, response);
            return;
        }
        JsonObject obj = jsonMsg.as<JsonObject>();
        processV2(obj, response);
        return;
    }
    processV1(message, response);
}

void ProtocolProcessor::processV1(const char* message, String& response) {
    String msgStr(message);
    int sep = msgStr.indexOf(':');
    String target, cmd;
    if (sep > 0) {
        target = msgStr.substring(0, sep);
        cmd = msgStr.substring(sep + 1);
    } else {
        target = "";
        cmd = msgStr;
    }
    if (target.length() == 0) {
        response = "ERROR: No target specified";
        return;
    }
    Feature* feature = registry.getFeature(target.c_str());
    if (feature) {
        feature->handleV1(cmd, response);
    } else {
        response = "ERROR: Feature '" + target + "' not found";
    }
}

void ProtocolProcessor::processV2(JsonObject& msg, String& response) {
    respDoc.clear();
    if (!msg.containsKey("v") || !msg.containsKey("type") || !msg.containsKey("target") || !msg.containsKey("action")) {
        respDoc["status"] = "error";
        respDoc["error"] = "Missing required fields: v, type, target, action";
        serializeJson(respDoc, response);
        return;
    }
    String target = msg["target"].as<String>();
    Feature* feature = registry.getFeature(target.c_str());
    if (feature) {
        JsonObject respObj = respDoc.to<JsonObject>();
        feature->handleV2(msg, respObj);
        serializeJson(respObj, response);
    } else {
        respDoc["status"] = "error";
        respDoc["error"] = String("Feature '") + target + "' not found";
        serializeJson(respDoc, response);
    }
}
