#ifndef HDLCFRAME_H
#define HDLCFRAME_H

#include <cstddef>
#include <cstdint>

const int HDLC_MAX_PAYLOAD = 0xFFFF;
const int HDLC_PDU_HEADER_SIZE = 3; // CHANNEL + FRAME_SIZE
const int HDLC_MARKER_SIZE = 2; // START / STOP MARKER
const int HDLC_CRC16_SIZE = 2; // CRC SIZE
// const int HDLC_FRAGMENTATION_FLAG = 0x80; not supported

const uint8_t HDLC_BOUNDARY_MARKER = (uint8_t) 0x7E;
const uint8_t HDLC_ESCAPE_MARKER  = (uint8_t) 0x7D;

const int HDLC_HEADER_SIZE = HDLC_MARKER_SIZE + HDLC_PDU_HEADER_SIZE + HDLC_CRC16_SIZE;

class HDLCFrame {
public:
    enum FrameState {
        INIT_SYNC = 0,
        CHANNEL = 1,
        SIZE_HIGH = 2,
        SIZE_LOW = 3,
        CRC_HIGH = 4,
        CRC_LOW = 5,
        END_SYNC = 6,
        DATA = HDLC_HEADER_SIZE
    };

    enum FrameMode {
        HDLC_IDLE,
        HDLC_SEND,
        HDLC_RECEIVE,
        HDLC_DONE,
        HDLC_ERROR_UNKNOWN_STATE,
        HDLC_ERROR_UNEXPECTED_BOUNDARY,
        HDLC_ERROR_BUFFER_TOO_SMALL,
        HDLC_ERROR_LENGTH_MISMATCH,
        HDLC_ERROR_CRC_MISMATCH
    };

    HDLCFrame();
    uint8_t *consume(uint16_t &size);
    size_t produce(uint8_t *data, uint16_t size);
    bool get(uint8_t &data);
    bool put(uint8_t &data);
    bool ready();
    FrameMode getMode(); 
    void setChannel(uint8_t ch) {
        channel = ch;
    }

    uint8_t getChannel() const {
        return channel;
    }

private:
    uint8_t *tx_data_buffer;
    uint8_t *rx_data_buffer;
    uint16_t pos;
    uint16_t len;
    uint8_t encoded_len;
    uint16_t crc;
    bool escape;
    uint8_t channel;
    FrameState state;
    FrameMode mode;
};

#endif // HDLCFRAME_H