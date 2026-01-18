#include <boards/M5AtomS3.h>

#ifdef ARDUINO_M5STACK_ATOMS3

namespace enableit {

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

static M5AtomS3 _board;
Board& board = _board;

Display& display = _board.getDisplay();

} // namespace enableit

#endif