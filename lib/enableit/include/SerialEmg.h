#ifndef S_EMG_H
#define S_EMG_H

#include "ADS129X.h"

//Channel
#define WINDOWLENGTH 200
#define STEPSIZE 75

/*
pwdn 3
rset 2
start 4
drdy 5
cs 10
*/
//LDA
#define NUMCLASSES 5
#define NUMFEATS (NUM_CHANNELS * 4)
#define PREVIOUSCLASSES 8
#define MLDA_ADDRESS 3
#define CLDA_ADDRESS 1024
#define STATUS_ADDRESS 2
#define THRESHOLD_MULT 1.15

//Proportional Control
#define PMINTHRESH 10
#define PMAXTHRESH 70

class SerialEmg {
public:
    typedef enum {
        RATE_16K,
        RATE_8K,
        RATE_4K,
        RATE_2K,
        RATE_1K,
        RATE_500,
        RATE_250
    } DATA_RATE;

    typedef enum {
        SRC_ELECTRODE,
        SRC_NOISE,
        SRC_SUPPLY,
        SRC_TEMPERATURE,
        SRC_TEST,
        SRC_RLD_MEAS,
        SRC_RLD_POS,
        SRC_RLD_NEG
    } CHANNEL_SRC;

    typedef enum {
        GAIN_1X,
        GAIN_2X,
        GAIN_3X,
        GAIN_4X,
        GAIN_6X,
        GAIN_8X,
        GAIN_12X
    } CHANNEL_GAIN;

    typedef enum {
        SIG_2HZ,
        SIG_1HZ,
        SIG_DC
    } TEST_SIGNAL;

    SerialEmg();

    void init(bool start = true);

    void setRate(DATA_RATE r);
    void setSrc(int channel, CHANNEL_SRC src);
    void setGain(int channel, CHANNEL_GAIN gain);
    void setTestMode(bool enable_2x, TEST_SIGNAL signal);
    void setBuffer(int size);

    void enable(int channel);
    void disable(int channel);

    void start();
    void streaming(bool on);
    void irq(bool on);
    int avail();
    void stop();
        
    bool readData(long* data);

    void write(byte reg, byte value);
    byte read(byte reg);

    int getMaxChannels() { return maxChannels; }
    long getStatus() { return ADS.getStatus(); }
    int getTicks() { return ADS.getTicks(); }
    byte getChannelState() { return ADS.getChannelStatus(); }
    BufferProducer *getBufferProducer() { return ADS.getBufferProducer(); }

    void poll() {
        ADS.poll();
    }
        
private:
    ADS129X ADS;
    //long buffer[NUM_CHANNELS + 1];
    int maxChannels;
    TEST_SIGNAL testSignal;
    DATA_RATE   rate;
    bool active;
    int bufferDepth;
};

#endif
