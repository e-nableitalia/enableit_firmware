#include <BLE2902.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "BtServer.h"
#include "BleCharacteristicHandler.h"
#include <algorithm>
#include "BleUuids.h"

namespace enableit
{

    // -------------------- BLE TX queue (notify from a safe task) --------------------

    class BtServer::CharacteristicCallback : public BLECharacteristicCallbacks
    {
    public:
        BleCharacteristicHandler *handler;
        std::string binding_key;

        CharacteristicCallback(BleCharacteristicHandler *h, std::string key)
            : handler(h), binding_key(key) {}

        void onWrite(BLECharacteristic *ch) override
        {
            if (handler)
                handler->onWrite(ch);
        }
        void onRead(BLECharacteristic *ch) override
        {
            if (handler)
                handler->onRead(ch);
        }
        void onStatus(BLECharacteristic *ch, Status s, uint32_t code) override
        {
            if (binding_key.empty() || !handler)
                return;
            if (s == Status::SUCCESS_NOTIFY || s == Status::SUCCESS_INDICATE)
            {
                auto binding = BtServer::instance().getCharBinding(binding_key);
                if (binding)
                {
                    binding->notifyEnabled = true;
                    handler->onSubscribe();
                }
            }
            else if (s == Status::ERROR_NOTIFY_DISABLED || s == Status::ERROR_INDICATE_DISABLED)
            {
                auto binding = BtServer::instance().getCharBinding(binding_key);
                if (binding)
                {
                    binding->notifyEnabled = false;
                    handler->onUnsubscribe();
                }
            }
        }
    };

    class BtServer::ServerCallback : public BLEServerCallbacks
    {
    public:
        void onConnect(BLEServer *pServer) override
        {
            log_i("BLE client connected.");
        }
        void onDisconnect(BLEServer *pServer) override
        {
            log_i("BLE client disconnected.");
            pServer->getAdvertising()->start();
        }
    };

    // Singleton instance accessor
    BtServer &BtServer::instance()
    {
        static BtServer instance;
        return instance;
    }

    // Make constructor private
    BtServer::BtServer()
        : server_(nullptr), service_(nullptr), bleTxQueue(nullptr)
    {
    }

    void BtServer::init(String name, String uuid)
    {
        log_d("BtServer::init called with name=%s, uuid=%s", name.c_str(), uuid.c_str());
        deviceName_ = name;
        serviceUuid_ = uuid;
        BLEDevice::init(deviceName_.c_str());
        log_d("BLEDevice initialized with name: %s", deviceName_.c_str());
        server_ = BLEDevice::createServer();
        log_d("BLEServer created: %p", server_);
        server_->setCallbacks(new ServerCallback());
        log_d("ServerCallback set.");

        // Use given UUID for the service
        service_ = server_->createService(serviceUuid_.c_str());
        log_d("BLEService created with uuid: %s, ptr: %p", serviceUuid_.c_str(), service_);

        log_i("eNableIt BLE core started for %s.", deviceName_.c_str());

        esp_bd_addr_t mac;
        esp_read_mac(mac, ESP_MAC_BT);

        log_i(
            "ESP32 BLE MAC: %02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    void BtServer::advertising()
    {
        log_d("BtServer::advertising called.");

        log_i("Starting BLE advertising for service UUID: %s", serviceUuid_.c_str());
        service_->start();
        log_d("Service started: %p", service_);

        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        log_d("BLEAdvertising obtained: %p", pAdvertising);

        pAdvertising->addServiceUUID(serviceUuid_.c_str());
        log_d("Service UUID added to advertising: %s", serviceUuid_.c_str());

        pAdvertising->setScanResponse(true);
        log_d("Scan response enabled.");
        pAdvertising->setMinPreferred(0x06); // helps with iPhone connections
        log_d("MinPreferred set to 0x06.");
        pAdvertising->setMinPreferred(0x12);
        log_d("MinPreferred set to 0x12.");

        BLEDevice::startAdvertising();
        log_d("BLEDevice::startAdvertising called.");
        server_->getAdvertising()->start();
        log_d("Server advertising started.");

        bleTxQueue = xQueueCreate(8, sizeof(BleTxMsg));
        log_d("bleTxQueue created: %p", bleTxQueue);
        xTaskCreatePinnedToCore([](void *arg)
                                {
        BtServer* self = static_cast<BtServer*>(arg);
        BleTxMsg msg{};
        log_d("BLE TX task started.");
        while (true) {
            if (self->bleTxQueue && xQueueReceive(self->bleTxQueue, &msg, portMAX_DELAY) == pdTRUE) {
                log_d("BLE TX message received: ch=%p, len=%u", msg.ch, msg.len);
                if (msg.ch) {
                    msg.ch->setValue(msg.data, msg.len);
                    msg.ch->notify();
                    log_d("BLE TX notified characteristic: %p", msg.ch);
                }
            }
        } }, "ble_tx", 4096, this, 1, nullptr, 1);

        log_i("eNableIt BLE core ready.");
    }

    void BtServer::end()
    {
        // Stop advertising
        if (server_)
        {
            server_->getAdvertising()->stop();
        }
        BLEDevice::stopAdvertising();

        // Remove all characteristics and clear bindings
        if (service_)
        {
            for (auto &kv : bindings_)
            {
                if (kv.second.characteristic)
                {
                    // BLEService does not provide removeCharacteristic, just delete the object
                    delete kv.second.characteristic;
                    kv.second.characteristic = nullptr;
                }
                if (kv.second.handler)
                {
                    delete kv.second.handler;
                    kv.second.handler = nullptr;
                }
            }
            bindings_.clear();
            server_->removeService(service_);
            delete service_;
            service_ = nullptr;
        }

        // Delete server
        if (server_)
        {
            delete server_;
            server_ = nullptr;
        }

        // Delete BLE TX queue
        if (bleTxQueue)
        {
            vQueueDelete(bleTxQueue);
            bleTxQueue = nullptr;
        }

        // Deinit BLE stack
        BLEDevice::deinit();
    }

    bool BtServer::registerCharacteristic(
        const char *uuid,
        uint32_t properties,
        BleCharacteristicHandler *handler)
    {
        log_d("registerCharacteristic: uuid=%s, properties=0x%08x, handler=%p", uuid ? uuid : "null", properties, handler);
        if (!service_ || !uuid)
        {
            log_d("registerCharacteristic: service_ or uuid is null");
            return false;
        }
        std::string key(uuid);
        if (bindings_.count(key))
        {
            log_d("registerCharacteristic: characteristic already registered for uuid=%s", uuid);
            return false; // Already registered
        }

        BLECharacteristic *ch = service_->createCharacteristic(uuid, properties);
        log_d("registerCharacteristic: created BLECharacteristic=%p for uuid=%s", ch, uuid);
        auto &binding = bindings_[key];
        binding.characteristic = ch;
        binding.handler = handler;
        binding.notifyEnabled = false;
        log_d("registerCharacteristic: binding created for uuid=%s", uuid);
        ch->setCallbacks(new CharacteristicCallback(handler, key));
        log_d("registerCharacteristic: callbacks set for uuid=%s", uuid);
        return true;
    }

    bool BtServer::notify(const char *uuid, const uint8_t *data, size_t len)
    {
        if (!bleTxQueue || !uuid || !data || len == 0)
            return false;
        auto it = bindings_.find(std::string(uuid));
        if (it == bindings_.end())
            return false;
        CharBinding &binding = it->second;
        if (!binding.characteristic || !binding.notifyEnabled)
            return false;
        BleTxMsg msg{};
        msg.ch = binding.characteristic;
        msg.len = std::min(len, BLE_TX_MAX);
        memcpy(msg.data, data, msg.len);
        (void)xQueueSend(bleTxQueue, &msg, 0);
        return true;
    }

    enableit::BtServer::CharBinding *BtServer::getCharBinding(const std::string &uuid)
    {
        auto it = bindings_.find(uuid);
        if (it != bindings_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

} // namespace enableit
