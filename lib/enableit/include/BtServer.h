#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <unordered_map>
#include <string>
#include "BleCharacteristicHandler.h"

#define SERVICE_UUID        "89d60870-9908-4472-8f8c-e5b3e6573cd1"

class BtServer {
public:
    static constexpr size_t BLE_TX_MAX = 180;

    BtServer();

    // Register a BLE characteristic with handler
    bool registerCharacteristic(
        const char* uuid,
        uint32_t properties,
        BleCharacteristicHandler* handler
    );

    // Safe async notify by UUID
    bool notify(const char* uuid, const uint8_t* data, size_t len);

    // BLE core members
    BLEServer* server_ = nullptr;
    BLEService* service_ = nullptr;

private:
    struct CharBinding {
        BLECharacteristic* characteristic;
        BleCharacteristicHandler* handler;
        bool notifyEnabled;
    };

    std::unordered_map<std::string, CharBinding> bindings_;

    struct BleTxMsg {
        BLECharacteristic* ch;
        uint8_t data[BLE_TX_MAX];
        size_t len;
    };

    QueueHandle_t bleTxQueue = nullptr;

    class CharacteristicCallback;
    class ServerCallback;
};