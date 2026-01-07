#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "BtServer.h"
#include "BleCharacteristicHandler.h"
#include <algorithm>
#include "BleUuids.h"

// -------------------- BLE TX queue (notify from a safe task) --------------------

class BtServer::CharacteristicCallback : public BLECharacteristicCallbacks {
public:
    BleCharacteristicHandler* handler;
    CharBinding* binding;

    CharacteristicCallback(BleCharacteristicHandler* h, CharBinding* b)
        : handler(h), binding(b) {}

    void onWrite(BLECharacteristic* ch) override {
        if (handler) handler->onWrite(ch);
    }
    void onRead(BLECharacteristic* ch) override {
        if (handler) handler->onRead(ch);
    }
    void onStatus(BLECharacteristic* ch, Status s, uint32_t code) override {
        if (!binding || !handler) return;
        if (s == Status::SUCCESS_NOTIFY || s == Status::SUCCESS_INDICATE) {
            binding->notifyEnabled = true;
            handler->onSubscribe();
        } else if (s == Status::ERROR_NOTIFY_DISABLED || s == Status::ERROR_INDICATE_DISABLED) {
            binding->notifyEnabled = false;
            handler->onUnsubscribe();
        }
    }
};

class BtServer::ServerCallback : public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override {
        log_i("BLE client connected.");
    }
    void onDisconnect(BLEServer* pServer) override {
        log_i("BLE client disconnected.");
        pServer->getAdvertising()->start();
    }
};

BtServer::BtServer()
{
    BLEDevice::init("eNableIt board");
    server_ = BLEDevice::createServer();
    server_->setCallbacks(new ServerCallback());

    // Use BleUuids::Console::SERVICE for the service UUID
    service_ = server_->createService(BleUuids::Console::SERVICE);

    service_->start();

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BleUuids::Console::SERVICE);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);

    BLEDevice::startAdvertising();
    server_->getAdvertising()->start();

    bleTxQueue = xQueueCreate(8, sizeof(BleTxMsg));
    xTaskCreatePinnedToCore([](void* arg) {
        BtServer* self = static_cast<BtServer*>(arg);
        BleTxMsg msg{};
        while (true) {
            if (self->bleTxQueue && xQueueReceive(self->bleTxQueue, &msg, portMAX_DELAY) == pdTRUE) {
                if (msg.ch) {
                    msg.ch->setValue(msg.data, msg.len);
                    msg.ch->notify();
                }
            }
        }
    }, "ble_tx", 4096, this, 1, nullptr, 1);

    log_i("eNableIt BLE core ready.");
}

bool BtServer::registerCharacteristic(
    const char* uuid,
    uint32_t properties,
    BleCharacteristicHandler* handler
) {
    if (!service_ || !uuid) return false;
    std::string key(uuid);
    if (bindings_.count(key)) return false; // Already registered

    BLECharacteristic* ch = service_->createCharacteristic(uuid, properties);
    auto& binding = bindings_[key];
    binding.characteristic = ch;
    binding.handler = handler;
    binding.notifyEnabled = false;
    ch->setCallbacks(new CharacteristicCallback(handler, &binding));
    return true;
}

bool BtServer::notify(const char* uuid, const uint8_t* data, size_t len) {
    if (!bleTxQueue || !uuid || !data || len == 0) return false;
    auto it = bindings_.find(std::string(uuid));
    if (it == bindings_.end()) return false;
    CharBinding& binding = it->second;
    if (!binding.characteristic || !binding.notifyEnabled) return false;
    BleTxMsg msg{};
    msg.ch = binding.characteristic;
    msg.len = std::min(len, BLE_TX_MAX);
    memcpy(msg.data, data, msg.len);
    (void)xQueueSend(bleTxQueue, &msg, 0);
    return true;
}

BtServer btserver;
