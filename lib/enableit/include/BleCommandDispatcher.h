#pragma once

#include "Feature.h"
#include "BleCharacteristicHandler.h"
#include "ProtocolProcessor.h"
#include "BtServer.h"
#include <Arduino.h>

#include <BLECharacteristic.h>

namespace enableit {

constexpr size_t JSON_DOC_SIZE = 256; // Adjust size as needed



// Base class for BLE command dispatchers (handles BLECharacteristicHandler logic, uuid/properties, and registration)
class BleCommandDispatcher : public BleCharacteristicHandler {
public:
    friend class RuntimeManager;

    BleCommandDispatcher(const char* uuid, uint32_t properties)
        : uuid(uuid), properties(properties) {}

    virtual ~BleCommandDispatcher() {}

    const char* getUuid() const { return uuid; }
    uint32_t getProperties() const { return properties; }

protected:
    const char* uuid;
    uint32_t properties;

private:
    // Register this dispatcher as a handler for its characteristic on the given BtServer
    // method is private to avoid direct calls
    bool init() {
        return BtServer::instance().registerCharacteristic(uuid, properties, this);
    }

};

// BLE handler and feature for V1 commands (single object, no extra pointers)
class BleV1CommandDispatcher : public FeatureV1, public BleCommandDispatcher {
public:
    BleV1CommandDispatcher(const char* uuid, uint32_t properties)
        : BleCommandDispatcher(uuid, properties) {}

    // FeatureV1 interface
    void handle(const String& cmd, String& response) override {
        // User must override this in derived class
        response = "NOT_IMPLEMENTED";
    }

    // BleCharacteristicHandler interface
    void onWrite(BLECharacteristic* ch) override {
        std::string value = ch->getValue();
        String response;
        handle(String(value.c_str()), response);
        ch->setValue(response.c_str());
    }

    void onRead(BLECharacteristic* ch) override {
        String response;
        handle("", response);
        ch->setValue(response.c_str());
    }
};

// BLE handler and feature for V2 commands (single object, no extra pointers)
class BleV2CommandDispatcher : public FeatureV2, public BleCommandDispatcher {
public:
    BleV2CommandDispatcher(const char* uuid, uint32_t properties)
        : BleCommandDispatcher(uuid, properties) {}

    // FeatureV2 interface
    void handle(const JsonObjectConst& msg, JsonObject& response) override {
        // User must override this in derived class
        response["status"] = "error";
        response["error"] = "NOT_IMPLEMENTED";
    }

    // BleCharacteristicHandler interface
    void onWrite(BLECharacteristic* ch) override {
        std::string value = ch->getValue();
        StaticJsonDocument<JSON_DOC_SIZE> doc;
        DeserializationError err = deserializeJson(doc, value.c_str());
        StaticJsonDocument<JSON_DOC_SIZE> respDoc;
        JsonObject respObj = respDoc.to<JsonObject>();
        if (err) {
            respObj["status"] = "error";
            respObj["error"] = err.c_str();
        } else {
            JsonObjectConst msg = doc.as<JsonObjectConst>();
            handle(msg, respObj);
        }
        String respStr;
        serializeJson(respObj, respStr);
        ch->setValue(respStr.c_str());
    }

    void onRead(BLECharacteristic* ch) override {
        StaticJsonDocument<JSON_DOC_SIZE> respDoc;
        JsonObject respObj = respDoc.to<JsonObject>();
        handle(JsonObjectConst(), respObj);
        String respStr;
        serializeJson(respObj, respStr);
        ch->setValue(respStr.c_str());
    }
};

} // namespace enableit
