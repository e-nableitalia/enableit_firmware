#pragma once

#ifdef ENABLEIT_BOARD_XIAO_ESP32S3

#include <Arduino.h>
#include <Board.h>

class XiaoESP32S3 : public Board {
   public:
    XiaoESP32S3(/* args */);
    ~XiaoESP32S3();

    MockDisplay Lcd = MockDisplay();

    void begin(bool LCDEnable = true) override;
    void end() override { /* implementazione vuota o spegnimento hardware */ }
    Display& getDisplay() override { return Lcd; }
};

#endif

// End of file M5AtomS3.h
