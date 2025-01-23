#ifndef _M5ATOMS3_H_
#define _M5ATOMS3_H_

#ifdef ARDUINO_M5Stack_ATOMS3

#include <M5Unified.h>
//#include <lvgl.h>
#include <Arduino.h>
//#include <Wire.h>
//#include <FastLED.h>
#include <JC_Button.h>
#include "M5Display.h"

//#include "utility/MPU6886.h"
//#include "utility/Button.h"
//#include "utility/LED_DisPlay.h"

class M5AtomS3 : public m5::M5Unified {
   public:
    M5AtomS3(/* args */);
    ~M5AtomS3();

    //MPU6886 IMU;
    // LCD
    M5Display Lcd = M5Display();

    void begin(bool LCDEnable = true);
};

extern M5AtomS3 AtomS3;
#define m5  AtomS3
#define lcd Lcd
#define imu IMU
#define Imu IMU

#endif

#endif
