#include <boards/XiaoESP32S3.h>

#ifdef ENABLEIT_BOARD_XIAO_ESP32S3

XiaoESP32S3::XiaoESP32S3() {}
XiaoESP32S3::~XiaoESP32S3() {}

void XiaoESP32S3::begin(bool lcdEnabled) {
    // Board-specific init if needed
}

void XiaoESP32S3::end() {
    // Board-specific deinit if needed
}

static XiaoESP32S3 _board;
Board& board = _board;

#endif
