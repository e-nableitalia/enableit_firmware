#pragma once

#include <Arduino.h>

namespace enableit {

enum ConsoleTransportState { CONSOLE_DISCONNECTED, CONSOLE_CONNECTED };
enum ConsolePriority { PRIORITY_SERIAL = 1, PRIORITY_BLE = 2, PRIORITY_TELNET = 3 };


class ConsoleWrapper;

class ConsoleTransport {
    friend class ConsoleWrapper;
public:
    ConsoleTransport(const String& name = "") : name(name) {}
    virtual ~ConsoleTransport() {}
    virtual void begin(int baudRate) = 0;
    virtual void attach(ConsoleWrapper* wrapper) { this->wrapper = wrapper; }
    virtual bool available() = 0;
    virtual int read() = 0;
    virtual size_t write(uint8_t c) = 0;
    virtual bool isConnected() = 0;
    virtual int peek() = 0;
    virtual ConsolePriority getPriority() const = 0;
    virtual bool needsPoll() const { return false; }
    virtual void poll() {}
    const String& getName() const { return name; }
protected:
    void notifyConnect();
    void notifyDisconnect();
    ConsoleWrapper* wrapper = nullptr;
    String name;
};

} // namespace enableit