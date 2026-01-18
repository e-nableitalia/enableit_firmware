#pragma once

#ifdef ARDUINO_XIAO_ESP32S3

#include <Arduino.h>
#include <Board.h>
#include <boards/WifiEsp32.h>

namespace enableit {

class XiaoConsoleTransport : public ConsoleTransport {
public:
    XiaoConsoleTransport() : ConsoleTransport("Xiao Console"), serial(Serial) {}
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

class XiaoESP32S3 : public Board {
public:
    XiaoESP32S3(/* args */);
    ~XiaoESP32S3();

    void begin(bool LCDEnable = true) override;
    void end() override;
    Display& getDisplay() override { return Lcd; }

    // HAL aggregate
    Wifi& wifi() override { return wifi_; }

    // Return the default hardware serial port
    ConsoleTransport& serial() override { return serialConsole_; }

private:
    MockDisplay Lcd;
    WifiEsp32 wifi_;
    XiaoConsoleTransport serialConsole_;
};

} // namespace enableit

#endif

// End of file XiaoESP32S3.h
