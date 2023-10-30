//
// Console debug functions
//
// Author: A.Navatta / e-Nable Italia

#ifndef CONSOLE_H

#define CONSOLE_H

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

class ConsoleWrapper : public Stream {

public:
    void init(int baudRate, bool debug, bool telnet = false);

    void debug(const bool dbg, const char *prefix, bool addlf, const char *function, const char *format_str, ...);
    void echo(uint8_t c);

    void pool();
    // Stream support functions
    virtual size_t write(uint8_t c);
    virtual int available();
    virtual int read();
    virtual int peek();

    // telnet management functions
    void enableTelnet(bool value);
    static void onTelnetConnect(String ip);
    static void onTelnetDisconnect(String ip);
    static void onTelnetReconnect(String ip);
    static void onTelnetConnectionAttempt(String ip);
    static void onTelnetInput(String ip);
private:
    void telnetSetup();
    bool debug_enabled;
    bool telnet;
    static bool telnet_connected;
};

extern ConsoleWrapper Console;

#endif // CONSOLE_H