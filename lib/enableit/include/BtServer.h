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
#include "BleUuids.h"

namespace enableit {

class BtServer {
public:
    static constexpr size_t BLE_TX_MAX = 180;

    struct CharBinding {
        BLECharacteristic* characteristic;
        BleCharacteristicHandler* handler;
        bool notifyEnabled;
    };

    // Singleton accessor
    static BtServer& instance();

    // Delete copy/move
    BtServer(const BtServer&) = delete;
    BtServer& operator=(const BtServer&) = delete;

    // Server init with service UUID
    void init(String name, String uuid);

    void advertising();

    void end();

    // Register a BLE characteristic with handler
    bool registerCharacteristic(
        const char* uuid,
        uint32_t properties,
        BleCharacteristicHandler* handler
    );

    // Safe async notify by UUID
    bool notify(const char* uuid, const uint8_t* data, size_t len);

    // Get CharBinding by UUID string key, or nullptr if not found
    CharBinding* getCharBinding(const std::string& uuid);

    bool isConnected() const { return connected_; }

    // BLE core members
    BLEServer* server_ = nullptr;
    BLEService* service_ = nullptr;

private:
    BtServer(); // Make constructor private

    std::unordered_map<std::string, CharBinding> bindings_;

    struct BleTxMsg {
        BLECharacteristic* ch;
        uint8_t data[BLE_TX_MAX];
        size_t len;
    };

    QueueHandle_t bleTxQueue = nullptr;

    class CharacteristicCallback;
    class ServerCallback;
    String deviceName_;
    String serviceUuid_;
    bool connected_ = false;
};

} // namespace enableit