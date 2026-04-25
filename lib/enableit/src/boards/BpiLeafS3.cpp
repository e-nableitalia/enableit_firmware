#include <boards/BpiLeafS3.h>

#ifdef ARDUINO_BPI_LEAF_S3

namespace enableit {

BpiLeafS3::BpiLeafS3() {}
BpiLeafS3::~BpiLeafS3() {}

void BpiLeafS3::begin(bool lcdEnabled) {
    // Board-specific init if needed
}

void BpiLeafS3::end() {
    // Board-specific deinit if needed
}

static BpiLeafS3 _board;
Board& board = _board;

} // namespace enableit

#endif
