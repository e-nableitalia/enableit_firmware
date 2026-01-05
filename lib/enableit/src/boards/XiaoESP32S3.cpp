#include <boards/XiaoESP32S3.h>

#ifdef ENABLEIT_BOARD_XIAO_ESP32S3

XiaoESP32S3::XiaoESP32S3() {}
XiaoESP32S3::~XiaoESP32S3() {}

void XiaoESP32S3::begin(bool LCDEnable) {
	if (LCDEnable) {
		Lcd.begin();
		Lcd.clear();
		Lcd.setCursor(1, 2);
	}
}

static MockDisplay _display;

Display& display = _display;

static XiaoESP32S3 _board;
Board& board = _board;

#endif
