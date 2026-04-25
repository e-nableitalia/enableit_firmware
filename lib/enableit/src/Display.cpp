#include <Display.h>

#include "qrcode.h"

namespace enableit {

void Display::qrcode(const char *string, uint16_t x, uint16_t y,
                       uint8_t width, uint8_t version) {
    // Create the QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(version)];
    qrcode_initText(&qrcode, qrcodeData, version, 0, string);

    // Top quiet zone
    uint8_t thickness   = width / qrcode.size;
    uint16_t lineLength = qrcode.size * thickness;
    uint8_t xOffset     = x + (width - lineLength) / 2;
    uint8_t yOffset     = y + (width - lineLength) / 2;
    fillRect(x, y, width, width, Color::WHITE);

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            uint8_t q = qrcode_getModule(&qrcode, x, y);
            if (q)
                fillRect(x * thickness + xOffset, y * thickness + yOffset,
                         thickness, thickness, Color::BLACK);
        }
    }
}

void Display::qrcode(const String &string, uint16_t x, uint16_t y,
                       uint8_t width, uint8_t version) {
    int16_t len = string.length() + 1;
    char buffer[len];
    string.toCharArray(buffer, len);
    qrcode(buffer, x, y, width, version);
}

} // namespace enableit