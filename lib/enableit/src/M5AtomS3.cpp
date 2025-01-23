#include <M5AtomS3.h>

#ifdef ARDUINO_M5Stack_ATOMS3

M5AtomS3::M5AtomS3() {
}

M5AtomS3::~M5AtomS3() {
}

void M5AtomS3::begin(bool LCDEnable) {
    if (LCDEnable) {
        Lcd.begin();
        Lcd.clear();
        Lcd.setCursor(1, 2);
    }
}


M5AtomS3 AtomS3;

#endif