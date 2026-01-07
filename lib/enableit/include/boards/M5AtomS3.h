#pragma once

#ifdef ENABLEIT_BOARD_M5STACK_ATOMS3

#include <M5Unified.h>
#include <Arduino.h>
#include <JC_Button.h>
#include "M5Display.h"
#include <Board.h>
#include <boards/WifiHalEsp32.h>

class M5AtomS3 : public m5::M5Unified, public Board {
public:
    M5AtomS3();
    ~M5AtomS3();

    M5Display Lcd = M5Display();

    void begin(bool LCDEnable = true) override;
    void end() override { /* implementazione vuota o spegnimento hardware */ }
    M5Display& getDisplay() override { return Lcd; }

    // HAL aggregate
    WifiHal& wifi() override { return wifi_; }

private:
    WifiHalEsp32 wifi_;
};

#endif

// End of file M5AtomS3.h
