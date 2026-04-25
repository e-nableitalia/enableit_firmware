#include <BleConsoleTransport.h>
#include <BtServer.h>
#include <BleUuids.h>

namespace enableit {

// BleConsoleTransport implementation
BleConsoleTransport::BleConsoleTransport(
    const std::string& rxCharUUID,
    const std::string& txCharUUID
)
    : rxUuid_(rxCharUUID), txUuid_(txCharUUID), ConsoleTransport("BLE Console")
{
    BtServer::instance().registerCharacteristic(
        rxUuid_.c_str(),
        /*PROPERTY_WRITE*/ 0x08 | /*PROPERTY_WRITE_NR*/ 0x04,
        this
    );
    BtServer::instance().registerCharacteristic(
        txUuid_.c_str(),
        /*PROPERTY_NOTIFY*/ 0x10,
        nullptr
    );

    Console.registerTransport(this, ConsolePriority::PRIORITY_BLE);
}

void BleConsoleTransport::pushRx(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        size_t nextHead = (rxHead_ + 1) % RX_BUF_SIZE;
        if (nextHead != rxTail_) {
            rxBuffer_[rxHead_] = data[i];
            rxHead_ = nextHead;
        }
        // else: drop byte if buffer full
    }
}

bool BleConsoleTransport::available() {
    return rxHead_ != rxTail_;
}

int BleConsoleTransport::read() {
    if (rxHead_ == rxTail_) return -1;
    uint8_t c = rxBuffer_[rxTail_];
    rxTail_ = (rxTail_ + 1) % RX_BUF_SIZE;
    return c;
}

int BleConsoleTransport::peek() {
    if (rxHead_ == rxTail_) return -1;
    return rxBuffer_[rxTail_];
}

size_t BleConsoleTransport::write(uint8_t c) {
    if (!subscribed_) return 0;
    uint8_t buf[1] = {c};
    if (BtServer::instance().notify(txUuid_.c_str(), buf, 1)) return 1;
    return 0;
}

bool BleConsoleTransport::isConnected() {
    return connected_;
}

// BleCharacteristicHandler interface
void BleConsoleTransport::onWrite(BLECharacteristic* c) {
    const uint8_t* data = c->getData();
    size_t len = c->getLength();
    pushRx(data, len);
}

void BleConsoleTransport::onSubscribe() {
    subscribed_ = true;
    connected_ = true;
    notifyConnect();
}

void BleConsoleTransport::onUnsubscribe() {
    subscribed_ = false;
    connected_ = false;
    notifyDisconnect();
}

// Example usage in your main setup/init (not inside this file):
// #include <BleUuids.h>
// BtServer btServer;
// static BleConsoleTransport bleConsole(
//     btServer,
//     BleUuids::Console::RX,
//     BleUuids::Console::TX
// );
// Console.registerTransport(&bleConsole, PRIORITY_BLE);

} // namespace enableit