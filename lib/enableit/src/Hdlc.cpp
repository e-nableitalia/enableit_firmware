#include "hdlc.h"
#include <stdexcept>
#include <thread>

HDLC::HDLC(HDLCTransport& transport, int num_attempts) : transport(transport), mode(IDLE) {
}

void HDLC::start() {
    if (mode == IDLE) {
        enterCommandMode();
    }
}

void HDLC::process() {
    if (mode == CONNECTED) {
        processConnected();
    } else if (mode == COMMAND) {
        processCommand();
    }
}

void HDLC::enterCommandMode() {
    mode = COMMAND;
   
}

void HDLC::enterConnectedMode() {
    mode = CONNECTED;
}

HDLCCommandMode::HDLCCommandMode(HDLCTransport &t, int a): transport(t),  max_attempts(a) {
    attempts = 0;
}

void HDLCCommandMode::process() {
    // process read data
    while (transport.available() > 0) {
        int byte = transport.read();
        switch (state) {
            case WAIT_ACK_A:
                if (byte == 'A') {
                    state = WAIT_ACK_C;
                }
                break;
            case WAIT_ACK_C:
                if (byte == 'C') {
                    state = WAIT_ACK_K;
                }
                break;
            case WAIT_ACK_K:
                if (byte == 'K') {
                    sendSYN();
                    state = WAIT_ACK2_A;
                }
                break;
            case WAIT_ACK2_A:
                if (byte == 'A') {
                    state = WAIT_ACK2_C;
                }
                break;
            case WAIT_ACK2_C:
                if (byte == 'C') {
                    state = WAIT_ACK2_K;
                }
                break;
            case WAIT_ACK2_K:
                if (byte == 'K') {
                    // write RDY -> READY
                    transport.write((uint8_t *)RDY,RDY_LEN);
                    state = WAIT_GO_G;
                }
                break;
            case WAIT_GO_G:
                if (byte == 'G') {
                    state = WAIT_GO_O;
                }
                break;
            case WAIT_GO_O:
                if (byte == 'O') {
                    state = WAIT_GO_SPACE;
                }
                break;
            case WAIT_GO_SPACE:
                if (byte == ' ') {
                    //enterConnectedMode();
                }
                break;
            default:
                if (byte == '\n' || byte == '\r') {
                    // Skip LF and CR characters
                    continue;
                } else {
                    state = WAIT_ACK_A;
                    attempts++;
                    if (attempts >= max_attempts) {
                        // Handle max attempts reached (e.g., reset or error handling)
                        //mode = IDLE;
                        // leave HDLC Command Mode
                        attempts = 0;
                    }
                }
                break;
        }
    }
}

void HDLCCommandMode::sendSYN() {
    transport.write((uint8_t *)SYN, SYN_LEN);
}


void HDLC::write() {
    // if (!tx_len) // buffer empty, nothing to write
    //     return;

    
    // size_t data_to_write = tx_len - tx_pos;

    // int tx_pos;
    // int tx_len;
    // int tx_crc;
    // bool tx_escape;

    // size_t available = transport.availableForWrite();
    // size_t to_write = std::min(available, data_to_write);
    // if (to_write > 0) {
    //     size_t written = transport.write(tx_buffer + written, to_write);
    //     tx_pos += written;
    // }
}