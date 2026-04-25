//
// Console debug functions
//
// Author: A.Navatta / e-Nable Italia

#pragma once

#include <vector>
#include <Arduino.h>
#include <Stream.h>
#include <ConsoleTransport.h>

namespace enableit {

// debug enabled
#define DEBUG
#define FORMAT_BUFFERSIZE 1024

#ifdef DEBUG
#define OUTNOLF(...) Console.debug(true, nullptr, false, __func__,  __VA_ARGS__)
#define OUT(...) Console.debug(false, nullptr, true, nullptr,  __VA_ARGS__)
#else
#define OUTNOLF(...) Console.debug(true, nullptr, false, __func__,  __VA_ARGS__)
#define OUT(...) Console.debug(false, nullptr, true, nullptr,  __VA_ARGS__)
#endif

class TelnetConsoleTransport : public ConsoleTransport {
public:
    TelnetConsoleTransport();
    void begin(int baudRate) override {}
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

} // namespace enableit