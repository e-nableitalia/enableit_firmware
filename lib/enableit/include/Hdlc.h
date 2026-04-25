#ifndef HDLC_H
#define HDLC_H

#include <vector>
#include <cstdint>
#include <chrono>
#include <queue>

const int NO_DATA = -1;
const int TIMEOUT = -2;
const int ERROR = -3;

const uint8_t SYN[] = "SYN";
const int SYN_LEN = 3;
const uint8_t RDY[] = "RDY";
const int RDY_LEN = 3;

const int DEFAULT_MAX_ATTEMPTS = 3;

const int MAX_HDLC_CHANNELS = 4;
const int HDLC_CONTROL_CHANNEL = 0;
const int HDLC_CONSOLE_CHANNEL = 1;

class HDLCTransport {
public:
    virtual ~HDLCTransport() = default;
    virtual int write(uint8_t byte) = 0;
    virtual int write(uint8_t *byte, int size) = 0;
    virtual int read() = 0;
    virtual size_t available() const = 0;
    virtual size_t availableForWrite() const = 0;
    virtual void setTimeout(std::chrono::milliseconds timeout) = 0;
};

class HDLCCommandMode {
public:
    enum State {
        WAIT_ACK_A,
        WAIT_ACK_C,
        WAIT_ACK_K,
        WAIT_ACK2_A,
        WAIT_ACK2_C,
        WAIT_ACK2_K,
        WAIT_GO_G,
        WAIT_GO_O,
        WAIT_GO_SPACE
    };

    HDLCCommandMode(HDLCTransport &transport, int attempts);
    virtual ~HDLCCommandMode() = default;
    void enter();
    void process();
private:
    void sendSYN();
    
    // command state
    HDLCTransport &transport;
    State state;
    int attempts;
    int max_attempts;
};

class HDLC {
public:
    enum Mode {
        IDLE,
        COMMAND,
        CONNECTED
    };



    HDLC(HDLCTransport& transport, int num_attempts = DEFAULT_MAX_ATTEMPTS);
    void start();
    void process();

private:
    void enterCommandMode();
    void enterConnectedMode();
    void processCommand();
    void processConnected();

    void write();

    HDLCTransport& transport;
    Mode mode;

    std::queue<std::vector<uint8_t>> tx_queue;
};

#endif // HDLC_H