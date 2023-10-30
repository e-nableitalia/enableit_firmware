#ifndef _M5DISPLAY_H_
#define _M5DISPLAY_H_

#include <Arduino.h>

#ifdef ARDUINO_M5Stack_ATOMS3

#include <M5GFX.h>

class M5Display : public m5gfx::M5GFX {
   public:
    M5Display();

    void begin();
 
    void qrcode(const char *string, uint16_t x = 5, uint16_t y = 45,
                uint8_t width = 70, uint8_t version = 7);
    void qrcode(const String &string, uint16_t x = 5, uint16_t y = 45,
                uint8_t width = 70, uint8_t version = 7);
};
#endif

#endif