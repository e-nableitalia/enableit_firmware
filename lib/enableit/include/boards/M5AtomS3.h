#pragma once

#ifdef ARDUINO_M5Stack_ATOMS3

#include <M5Unified.h>
#include <Arduino.h>
#include <Board.h>
#include <Display.h>
#include <boards/M5Display.h>
#include <boards/WifiEsp32.h>

namespace enableit {

class M5ConsoleTransport : public ConsoleTransport {
public:
    M5ConsoleTransport() : ConsoleTransport("M5 Console"), serial(Serial) {}
    void begin(int baudRate) override { serial.begin(baudRate); }
    bool available() override { return serial.available(); }
    int read() override { return serial.read(); }
    size_t write(uint8_t c) override { return serial.write(c); }
    bool isConnected() override { return true; }
    int peek() override { return serial.peek(); }
    ConsolePriority getPriority() const override { return PRIORITY_SERIAL; }
private:
    HWCDC& serial;
};

class M5AtomS3 : public m5::M5Unified, public Board {
public:
    M5AtomS3();
    ~M5AtomS3();

    void begin(bool LCDEnable = true) override;
    void end() override { /* implementazione vuota o spegnimento hardware */ }

    // Use fully qualified name for enableit::Display
    enableit::Display& getDisplay() override { return Lcd; }

    // HAL aggregate
    Wifi& wifi() override { return wifi_; }

    // Return the default hardware serial port
    ConsoleTransport& serial() override { return serialConsole_; }

    const char *name() const override { return "M5AtomS3"; }

private:
    M5Display Lcd;
    WifiEsp32 wifi_;
    M5ConsoleTransport serialConsole_;
};

} // namespace enableit

#endif

// End of file M5AtomS3.h
