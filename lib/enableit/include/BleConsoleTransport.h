#pragma once

#include <Arduino.h>
#include <Console.h>
#include <BtServer.h>
#include <BleCharacteristicHandler.h>

// Usage: pass BleUuids::Console::RX and BleUuids::Console::TX for UUIDs

namespace enableit {

class BleConsoleTransport : public ConsoleTransport, public BleCharacteristicHandler {
public:
    BleConsoleTransport(BtServer& btServer,
                        const std::string& rxUuid,
                        const std::string& txUuid);

    // ConsoleTransport interface
    bool available() override;
    int read() override;
    size_t write(uint8_t c) override;
    bool isConnected() override;
    int peek() override;
    ConsolePriority getPriority() const override { return PRIORITY_BLE; }
    bool needsPoll() const override { return false; }
    void poll() override {}

    // BleCharacteristicHandler interface
    void onWrite(class BLECharacteristic* c) override;
    void onSubscribe() override;
    void onUnsubscribe() override;

private:
    BtServer& btServer_;
    std::string rxUuid_;
    std::string txUuid_;
    bool connected_ = false;
    bool subscribed_ = false;

    // Minimal ring buffer for RX
    static constexpr size_t RX_BUF_SIZE = 128;
    uint8_t rxBuffer_[RX_BUF_SIZE];
    size_t rxHead_ = 0;
    size_t rxTail_ = 0;

    // Helper for buffer push
    void pushRx(const uint8_t* data, size_t len);
};

} // namespace enableit