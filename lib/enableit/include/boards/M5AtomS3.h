#pragma once

#ifdef ENABLEIT_BOARD_M5STACK_ATOMS3

#include <M5Unified.h>
//#include <lvgl.h>
#include <Arduino.h>
//#include <Wire.h>
//#include <FastLED.h>
#include <JC_Button.h>

#include "M5Display.h"
#include <Board.h>

//#include "utility/MPU6886.h"
//#include "utility/Button.h"
//#include "utility/LED_DisPlay.h"

class M5AtomS3 : public m5::M5Unified, public Board {
   public:
    M5AtomS3(/* args */);
    ~M5AtomS3();

    //MPU6886 IMU;
    // LCD
    M5Display Lcd = M5Display();

    void begin(bool LCDEnable = true) override;
    void end() override { /* implementazione vuota o spegnimento hardware */ }
    M5Display& getDisplay() override { return Lcd; }
};

#endif

// End of file M5AtomS3.h
