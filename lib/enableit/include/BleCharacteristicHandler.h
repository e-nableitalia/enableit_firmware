#pragma once

#include <string>

// Forward declaration only
class BLECharacteristic;

namespace enableit {

// BLE-agnostic handler interface
class BleCharacteristicHandler {
public:
    virtual ~BleCharacteristicHandler() {}

    virtual void onWrite(BLECharacteristic* ch) = 0;
    virtual void onRead(BLECharacteristic* ch) {}
    virtual void onSubscribe() {}
    virtual void onUnsubscribe() {}
};

} // namespace enableit

