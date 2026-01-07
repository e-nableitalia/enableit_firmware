#include <BleConsoleTransport.h>

// Example UUIDs for the BLE console characteristics
constexpr char CONSOLE_RX_UUID[] = "e3b8c7e0-1c7e-4e8d-9e4e-1a2b3c4d5e6f";
constexpr char CONSOLE_TX_UUID[] = "e3b8c7e1-1c7e-4e8d-9e4e-1a2b3c4d5e6f";

// BleConsoleTransport implementation
BleConsoleTransport::BleConsoleTransport(
    BtServer& btServer,
    const std::string& rxCharUUID,
    const std::string& txCharUUID
)
    : btServer_(btServer), rxUuid_(rxCharUUID), txUuid_(txCharUUID)
{
    // Register RX characteristic with this handler
    btServer_.registerCharacteristic(
        rxCharUUID.c_str(),
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR,
        this
    );
    // Register TX characteristic for output (no handler needed)
    btServer_.registerCharacteristic(
        txCharUUID.c_str(),
        BLECharacteristic::PROPERTY_NOTIFY,
        nullptr
    );

    // Register in Console with priority
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
    if (btServer_.notify(txUuid_.c_str(), buf, 1)) return 1;
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

// Example registration (place in your main setup/init, not inside the class file):
// BtServer btServer;
// static BleConsoleTransport bleConsole(btServer, CONSOLE_RX_UUID, CONSOLE_TX_UUID);
// Console.registerTransport(&bleConsole, PRIORITY_BLE);