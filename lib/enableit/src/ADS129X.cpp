/**
 * ADS129X.cpp
 *
 * Arduino library for the TI ADS129X series of analog-front-ends for
 * biopotential measurements (EMG/EKG/EEG).
 *
 * This library offers two modes of operation: polling and interrupt.
 * Polling mode should only be used in situations where multiple devices
 * share the SPI bus. Interrupt mode is much faster (8kSPS on a Teensy 3.1),
 * but starts receiving immediately when DRDY goes high.
 * The API is the same for both modes. To activate polling mode add
 *     #define ADS129X_POLLING
 * as first line to your sketch.
 *
 * Based on code by Conor Russomanno (https://github.com/conorrussomanno/ADS1299)
 * Modified by Ferdinand Keil
 */

#include "Arduino.h"

#include "ADS129X.h"
#include <SPI.h>
#include <mutex>
#include <Console.h>


std::mutex spi_mtx;

bool    ADS129X_isr;
int     ADS129X_CS;
long    ADS129X_data[9];
bool    ADS129X_newData;
bool    ADS129X_bufferedTransfer;
void    ADS129X_dataReadyISR();

// 8 channels + 1 status data, 24 bit, 3 byte data each
uint8_t ADS129X_isrBuffer[CHANNEL_DATA_SIZE * NUM_CHANNELS + CHANNEL_DATA_SIZE];
CircularBuffer *ADS129X_databuffer;

int isr_in = 0;
int isr_out = 0;
int isr_tick = 0;

String ADS129X::events[MAX_EVENTS];
int ADS129X::numEvents = 0;

  const char *sources[] = {
    "electrode",
    "noise/dc offset",
    "RLD meas",
    "supply",
    "temperature",
    "test",
    "RLD positive",
    "RLD negative"
  };

  const char *gains[] {
      "Gain 6x",
      "Gain 1x",
      "Gain 2x",
      "Gain 3x",
      "Gain 4x",
      "Gain 8x",
      "Gain 12x"
    };

/**
 * Initializes ADS129x library.
 * @param _DRDY data ready pin
 * @param _CS   chip-select pin
 */
ADS129X::ADS129X() {
   
}

void ADS129X::init(int _DRDY, int _CS) {
    DBG("ADS Setup");
    // initialize the  data ready and chip select pins:
    DRDY = _DRDY;
    CS = _CS;
    ADS129X_CS = _CS;  
    DBG("SPI Setup");
    // SPI Setup
    SPI.begin(EMG_SCLK, EMG_MISO, EMG_MOSI, EMG_CS1);

    // set clock divider
    SPI.setClockDivider(SPI_CLOCK_DIV2);

    // set data mode
    SPI.setDataMode(SPI_MODE1); //clock polarity = 0; clock phase = 1 (pg. 8)

    // set bit order
    SPI.setBitOrder(MSBFIRST); //SPI data format is MSB (pg. 25)

    DBG("PIN Setup");
    // clk provided by internal clock
    pinMode(EMG_CLK, INPUT);
    pinMode(EMG_START,OUTPUT);
    //pinMode(DRDY, INPUT_PULLUP);
    pinMode(DRDY, INPUT);
    pinMode(CS, OUTPUT);
    // start triggered by start command
    digitalWrite(EMG_START,LOW);
    // cs high -> ads disabled
    digitalWrite(CS, HIGH);
    
    DBG("ADS Setup complete");
}

/**
 * Reset Registers to Default Values.
 */
void ADS129X::RESET() {
    ADS129xCommandLock lock(this, false);

    sendCommand(ADS129X_CMD_RESET);
    delay(10); //must wait 18 tCLK cycles to execute this command (Datasheet, pg. 38)

    // start triggered by start command
    digitalWrite(EMG_START,LOW);
    // cs high -> ads disabled
    digitalWrite(CS, HIGH);

    // stop streaming
    sendCommand(ADS129X_CMD_SDATAC);

    // init variables
    ADS129X_newData = false;

    dataMode = false;

    ADS129X_bufferedTransfer = false;
    ADS129X_databuffer = &dataBuffer;
    for (int i = 0; i < NUM_CHANNELS + 1; i++) {
        ADS129X_data[i] = 0;
    }
    for (int j = 0; j < NUM_CHANNELS * CHANNEL_DATA_SIZE; j++) {
        ADS129X_isrBuffer[j] = 0;
    }
}

//System Commands
void ADS129X::sendCommand(byte cmd) {
    DBG("Sending command[0x%x]", cmd);    
    digitalWrite(CS, LOW); //Low to communicate
    SPI.transfer(cmd);
    delayMicroseconds(2);
    digitalWrite(CS, HIGH); //High to end communication
    delayMicroseconds(2);  //must way at least 4 tCLK cycles before sending another command (Datasheet, pg. 38)

}

/**
 * Exit Standby Mode.
 */
void ADS129X::WAKEUP() {
    ADS129xCommandLock lock(this);

    sendCommand(ADS129X_CMD_WAKEUP);
}

/**
 * Enter Standby Mode.
 */
void ADS129X::STANDBY() {
    ADS129xCommandLock lock(this);

    sendCommand(ADS129X_CMD_STANDBY);
}

/**
 * Enable Read Data Continuous mode (default).
 */
void ADS129X::RDATAC() {
    if (dataMode)
        return;
    
    DBG("RDATAC");

    dataMode = true;
    START();
    enableIrq();
    sendCommand(ADS129X_CMD_RDATAC);
}

/**
 * Stop Read Data Continuously mode.
 */
void ADS129X::SDATAC() {
    DBG("SDATAC");
    if (dataMode) {
        dataMode = false;
        disableIrq();
    }
    STOP();
    sendCommand(ADS129X_CMD_SDATAC);
}

/**
 * Start/restart (synchronize) conversions.
 */
void ADS129X::START() {
    DBG("Start");

    digitalWrite(EMG_START,HIGH);
    //sendCommand(ADS129X_CMD_START);
}

/**
 * Stop conversion.
 */
void ADS129X::STOP() {
    DBG("Stop");

    //sendCommand(ADS129X_CMD_STOP);
    digitalWrite(EMG_START,LOW);
}

void ADS129X::enableIrq() {
    DBG("Attaching interrupt");
    ADS129X_isr = true;
    attachInterrupt(DRDY, ADS129X_dataReadyISR, FALLING);
}

void ADS129X::disableIrq() {
    if (ADS129X_isr) {
        DBG("Detach interrupt");
        ADS129X_isr = false;
        detachInterrupt(DRDY);
    }
}

/**
 * Read data by command; supports multiple read back.
 */
bool ADS129X::RDATA(long *buffer) {
    if (ADS129X_isr) {
        return false;
    } else if (digitalRead(DRDY) == LOW) {
        digitalWrite(CS, LOW);
        SPI.transfer(ADS129X_CMD_RDATA);
        //delayMicroseconds(2);
        for(int i = 0; i<9; i++){
            long dataPacket = 0;
            for(int j = 0; j<3; j++){
                byte dataByte = SPI.transfer(0x00);
                dataPacket = (dataPacket<<8) | dataByte;
            }
            if (dataPacket & 0x800000)
                buffer[i] = 0xFF000000 | dataPacket;
            else
                buffer[i] = dataPacket;
        }
        digitalWrite(CS, HIGH);
        return true;
    }
    return false;
}

/**
 * Read register at address _address.
 * @param  _address register address
 * @return          value of register
 */
byte ADS129X::RREG(byte _address) {
    
    byte opcode1 = ADS129X_CMD_RREG | (_address & 0x1F); //001rrrrr; _RREG = 00100000 and _address = rrrrr
   
    ADS129xCommandLock lock(this);
 
    digitalWrite(CS, LOW); //Low to communicate
    SPI.transfer(ADS129X_CMD_SDATAC); //SDATAC
    SPI.transfer(opcode1); //RREG
    SPI.transfer(0x00); //opcode2
    delayMicroseconds(1);
    byte data = SPI.transfer(0x00); // returned byte should match default of register map unless edited manually (Datasheet, pg.39)
    delayMicroseconds(2);
    digitalWrite(CS, HIGH); //High to end communication

    return data;
}

/**
 * Read _numRegisters register starting at address _address.
 * @param _address      start address
 * @param _numRegisters number of registers
 * @param data          pointer to data array
 */
void ADS129X::RREG(byte _address, byte _numRegisters, byte *_data) {
    byte opcode1 = ADS129X_CMD_RREG | (_address & 0x1F); //001rrrrr; _RREG = 00100000 and _address = rrrrr

    ADS129xCommandLock lock(this);
    
    digitalWrite(CS, LOW); //Low to communicated
    SPI.transfer(ADS129X_CMD_SDATAC); //SDATAC
    SPI.transfer(opcode1); //RREG
    SPI.transfer(_numRegisters-1); //opcode2
    for(byte i = 0; i < _numRegisters; i++){
        *(_data+i) = SPI.transfer(0x00); // returned byte should match default of register map unless previously edited manually (Datasheet, pg.39)
    }
    delayMicroseconds(2);
    digitalWrite(CS, HIGH); //High to end communication   
}

/**
 * Write register at address _address.
 * @param _address register address
 * @param _value   register value
 */
void ADS129X::WREG(byte _address, byte _value) {
    //DBG("Write reg[%d], value[%d]", _address, _value);
    byte opcode1 = ADS129X_CMD_WREG | (_address & 0x1F); //001rrrrr; _RREG = 00100000 and _address = rrrrr

    ADS129xCommandLock lock(this);

    digitalWrite(CS, LOW); //Low to communicate
    SPI.transfer(opcode1);
    SPI.transfer(0x00); // opcode2; only write one register
    SPI.transfer(_value);
    delayMicroseconds(2);
    digitalWrite(CS, HIGH); //Low to communicate
   
}

/**
 * Read device ID.
 * @return device ID
 */
byte ADS129X::getDeviceId() {
    ADS129xCommandLock lock(this);

    digitalWrite(CS, LOW); //Low to communicate
    SPI.transfer(ADS129X_CMD_RREG); //RREG
    SPI.transfer(0x00); //Asking for 1 byte
    byte data = SPI.transfer(0x00); // byte to read (hopefully 0b???11110)
    delayMicroseconds(2);
    digitalWrite(CS, HIGH); //Low to communicate
    
    DBG("Device Id[%d]", data);

    return data;
}

void ADS129X::setBufferedTransfer(int size) {
    if (size > 0) {
        DBG("Buffered transfer enabled, size[%d]", size * NUM_CHANNELS);
        ADS129X_bufferedTransfer = true;
        dataBuffer.size(size * NUM_CHANNELS, CHANNEL_DATA_SIZE, CHANNEL_DATA_SIZE * NUM_CHANNELS);
    } else {
        DBG("Buffered transfer disabled");
        ADS129X_bufferedTransfer = false;
        dataBuffer.size(0, 0, 0);
    }
}

long ADS129X::getStatus() {
    return ADS129X_data[0];
}

int ADS129X::getTicks() {
    return isr_tick;
}

/**
 * Interrupt that gets called when DRDY goes HIGH.
 * Transfers data and sets a flag.
 */
void ADS129X_dataReadyISR() {

    isr_tick++;

    digitalWrite(ADS129X_CS, LOW);
 
    isr_in++;

    if (ADS129X_bufferedTransfer) {
        // read data
        for (int i = 0; i < NUM_CHANNELS * CHANNEL_DATA_SIZE + CHANNEL_DATA_SIZE; i++) {
            ADS129X_isrBuffer[i] = SPI.transfer(0x00);
        }
        // SPI.transfer(ADS129X_isrBuffer, NUM_CHANNELS * CHANNEL_DATA_SIZE + CHANNEL_DATA_SIZE);

        // copy status
        ((char*) ADS129X_data)[0*4+2] = ADS129X_isrBuffer[0];
        ((char*) ADS129X_data)[0*4+1] = ADS129X_isrBuffer[1];
        ((char*) ADS129X_data)[0*4+0] = ADS129X_isrBuffer[2];

//        int *data = (int *)(ADS129X_isrBuffer + 3);
//        *data = isr_in;
    
        ADS129X_databuffer->produce(ADS129X_isrBuffer + 3,NUM_CHANNELS * CHANNEL_DATA_SIZE);
    } else {     
        // channel 1
        ((char*) ADS129X_data)[1*4+3] = 0;
        ((char*) ADS129X_data)[1*4+2] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[1*4+1] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[1*4+0] = SPI.transfer(0x00);
        // channel 2
        ((char*) ADS129X_data)[2*4+3] = 0;
        ((char*) ADS129X_data)[2*4+2] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[2*4+1] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[2*4+0] = SPI.transfer(0x00);
        // channel 3
        ((char*) ADS129X_data)[3*4+3] = 0;
        ((char*) ADS129X_data)[3*4+2] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[3*4+1] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[3*4+0] = SPI.transfer(0x00);
        // channel 4
        ((char*) ADS129X_data)[4*4+3] = 0;
        ((char*) ADS129X_data)[4*4+2] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[4*4+1] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[4*4+0] = SPI.transfer(0x00);
        // channel 5
        ((char*) ADS129X_data)[5*4+3] = 0;
        ((char*) ADS129X_data)[5*4+2] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[5*4+1] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[5*4+0] = SPI.transfer(0x00);
        // channel 6
        ((char*) ADS129X_data)[6*4+3] = 0;
        ((char*) ADS129X_data)[6*4+2] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[6*4+1] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[6*4+0] = SPI.transfer(0x00);
        // channel 7
        ((char*) ADS129X_data)[7*4+3] = 0;
        ((char*) ADS129X_data)[7*4+2] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[7*4+1] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[7*4+0] = SPI.transfer(0x00);
        // channel 8
        ((char*) ADS129X_data)[8*4+3] = 0;
        ((char*) ADS129X_data)[8*4+2] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[8*4+1] = SPI.transfer(0x00);
        ((char*) ADS129X_data)[8*4+0] = SPI.transfer(0x00);

        ADS129X_newData = true;
    }

    digitalWrite(ADS129X_CS, HIGH);

    isr_out++;
}

int ADS129X::avail() {
    return dataBuffer.avail();
}

/**
 * Receive data when in continuous read mode.
 * @param buffer buffer for received data
 * @return true when received data
 */
boolean ADS129X::getData(long *buffer) {
    if (ADS129X_isr) {
        if (ADS129X_newData) {
            ADS129X_newData = false;
            for (int i = 1; i < 9; i++) {
                buffer[i] = ADS129X_data[i];
            }
            return true;
        }
        return false;
    } else {
        return RDATA(buffer);
    }
}

/**
 * Configure channel _channel.
 * @param _channel   channel (1-8)
 * @param _powerDown power down (true, false)
 * @param _gain      gain setting
 * @param _mux       mux setting
 */
void ADS129X::configChannel(byte _channel, boolean _powerDown, byte _gain, byte _mux) {
    byte value = ((_powerDown & 1)<<7) | ((_gain & 7)<<4) | (_mux & 7);
    DBG("Channel[%d], Enabled[%s], Source[%s], Gain[%s]", _channel, _powerDown ? "false" : "true", sources[_mux], gains[_gain]);

    WREG(ADS129X_REG_CH1SET + _channel, value);
}

void ADS129X::addEvent(const char *evt) {
    if (numEvents < MAX_EVENTS) {
        events[numEvents++] = evt;
    }
}

void ADS129X::poll() {
    for (int i = 0; i < numEvents; i++) {
        DBG("Event[%s]", events[i].c_str());
    }
    numEvents = 0;

/* // DMS
   // Debug disabled
    if (isr_tick)
        DBG("isr ticks[%d], isr in[%d], isr out[%d]", isr_tick, isr_in, isr_out);
    
    dataBuffer.dump();
*/
}

BufferProducer *ADS129X::getBufferProducer() {
    return ADS129X_databuffer;
}