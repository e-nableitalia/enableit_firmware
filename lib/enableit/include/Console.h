//
// Console debug functions
//
// Author: A.Navatta / e-Nable Italia

#pragma once

#include <vector>
#include <Arduino.h>
#include "Stream.h"

// debug enabled
#define DEBUG
#define FORMAT_BUFFERSIZE 1024

//void debug_enable(bool enabled);
//void console_debug(const bool dbg, const char *info, bool addcr, const char *function, const char *format_str, ...);

#ifdef DEBUG
#define DBG(...) Console.debug(true, "DBG", true, __func__,  __VA_ARGS__)
#define DBGNOLF(...) Console.debug(true, "DBG", false, __func__,  __VA_ARGS__)
#define LOG(...) Console.debug(true, "LOG", true, __func__,  __VA_ARGS__)
#define ERR(...) Console.debug(false, "ERR", true, __func__,  __VA_ARGS__)
#define OUTNOLF(...) Console.debug(true, nullptr, false, __func__,  __VA_ARGS__)
#define OUT(...) Console.debug(false, nullptr, true, nullptr,  __VA_ARGS__)
#else
#define DBG(...)
#define DBGNOLF(...)
#define LOG(...) Console.debug(true, "LOG", true, __func__,  __VA_ARGS__)
#define ERR(...) Console.debug(false, "ERR", true, __func__,  __VA_ARGS__)
#define OUTNOLF(...) Console.debug(true, nullptr, false, __func__,  __VA_ARGS__)
#define OUT(...) Console.debug(false, nullptr, true, nullptr,  __VA_ARGS__)
#endif

enum ConsoleTransportState { CONSOLE_DISCONNECTED, CONSOLE_CONNECTED };
enum ConsolePriority { PRIORITY_SERIAL = 1, PRIORITY_BLE = 2, PRIORITY_TELNET = 3 };

class ConsoleWrapper;

class ConsoleTransport {
public:
    virtual ~ConsoleTransport() {}
    virtual void attach(ConsoleWrapper* wrapper) { this->wrapper = wrapper; }
    virtual bool available() = 0;
    virtual int read() = 0;
    virtual size_t write(uint8_t c) = 0;
    virtual bool isConnected() = 0;
    virtual int peek() = 0;
    virtual ConsolePriority getPriority() const = 0;
    virtual bool needsPoll() const { return false; }
    virtual void poll() {}
protected:
    void notifyConnect();
    void notifyDisconnect();
    ConsoleWrapper* wrapper = nullptr;
};

class SerialConsoleTransport : public ConsoleTransport {
public:
    SerialConsoleTransport(HardwareSerial& serial) : serial(serial) {}
    bool available() override { return serial.available(); }
    int read() override { return serial.read(); }
    size_t write(uint8_t c) override { return serial.write(c); }
    bool isConnected() override { return true; }
    int peek() override { return serial.peek(); }
    ConsolePriority getPriority() const override { return PRIORITY_SERIAL; }
private:
    HardwareSerial& serial;
};

class TelnetConsoleTransport : public ConsoleTransport {
public:
    TelnetConsoleTransport();
    bool available() override;
    int read() override;
    size_t write(uint8_t c) override;
    bool isConnected() override;
    int peek() override;
    ConsolePriority getPriority() const override { return PRIORITY_TELNET; }
    bool needsPoll() const override { return true; }
    void poll() override;
    void enable(bool enable);
    // Telnet event handlers
    void onConnect();
    void onDisconnect();
    void onReconnect();
    void onConnectionAttempt();
    void onInput(String str);

    // Static callback wrappers for ESPTelnetStream
    static void onConnectStatic(String ip);
    static void onDisconnectStatic(String ip);
    static void onReconnectStatic(String ip);
    static void onConnectionAttemptStatic(String ip);
    static void onInputReceivedStatic(String str);
    static TelnetConsoleTransport* instance() { return _instance; };

private:
    bool connected = false;
    static TelnetConsoleTransport* _instance;
};

class ConsoleWrapper : public Stream {
public:
    void init(int baudRate, bool debug, bool telnet = false);
    void debug(const bool dbg, const char *prefix, bool addlf, const char *function, const char *format_str, ...);
    void echo(uint8_t c);
    void poll();
    // Stream support functions
    virtual size_t write(uint8_t c);
    virtual int available();
    virtual int read();
    virtual int peek();

    // Transport registration
    void registerTransport(ConsoleTransport* transport, ConsolePriority priority);
    void unregisterTransport(ConsoleTransport* transport);

    // Event notification from transports
    void onTransportConnected(ConsoleTransport* transport);
    void onTransportDisconnected(ConsoleTransport* transport);

    // Select active transport
    void selectActiveTransport();
    ConsoleTransport* getActiveTransport() const;

private:
    struct TransportEntry {
        ConsoleTransport* transport;
        ConsolePriority priority;
        ConsoleTransportState state;
    };
    std::vector<TransportEntry> transports;
    ConsoleTransport* activeTransport = nullptr;
    bool debug_enabled;
};

extern ConsoleWrapper Console;