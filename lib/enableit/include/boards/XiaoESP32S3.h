#pragma once

#ifdef ENABLEIT_BOARD_XIAO_ESP32S3

#include <Arduino.h>
#include <Board.h>
#include <boards/WifiHalEsp32.h>

class XiaoESP32S3 : public Board {
   public:
    XiaoESP32S3(/* args */);
    ~XiaoESP32S3();

    MockDisplay Lcd = MockDisplay();

    void begin(bool LCDEnable = true) override;
    void end() override;
    Display& getDisplay() override { return Lcd; }

    // HAL aggregate
    WifiHal& wifi() override { return wifi_; }

   private:
    WifiHalEsp32 wifi_;
};

#endif

// End of file M5AtomS3.h
