#include <boards/M5Display.h>

#ifdef ENABLEIT_BOARD_M5STACK_ATOMS3

#include "qrcode.h"

namespace enableit {

M5Display::M5Display() : m5gfx::M5GFX() {
 
}

void M5Display::begin() {
    m5gfx::M5GFX::begin();
    m5gfx::M5GFX::fillScreen(0);
    m5gfx::M5GFX::setRotation(1);
}

} // namespace enableit

#endif