/**
 * ADS129X.cpp
 *
 * Updated Arduino library for the TI ADS129X series of analog-front-ends for
 * biopotential measurements (EMG/EKG/EEG).
 *
 * This library offers two modes of operation: polling and interrupt.
 * Polling mode should only be used in situations where multiple devices
 * share the SPI bus. Interrupt mode is much faster (8kSPS on a Teensy 3.1),
 * but starts receiving immediately when DRDY goes high.
 *
 * Based on code by Conor Russomanno (https://github.com/conorrussomanno/ADS1299)
 * Modified by Ferdinand Keil
 * Modified by A. Navatta (enableIT):
 * - Added configurable streaming and data mode
 * - Added buffered transfer using freertos CircularBuffer
 * - Added event logging
 * - Added polling method
 * - Added mutex for SPI access
 */

#include "Arduino.h"

#include "ADS129X.h"
#include <SPI.h>
#include <mutex>
#include <Console.h>

std::mutex spi_mtx;


bool ADS129X_newData;
bool ADS129X_bufferedTransfer;

void ADS129X_dataReadyISR();

bool channelEnabled[NUM_CHANNELS];

int isr_in = 0;
int isr_out = 0;
int isr_tick = 0;

int ADS129X::ADS129X_CS;
CircularBuffer ADS129X::ADS129X_dataBuffer;
// spi read buffer
uint8_t ADS129X::ADS129X_tempBuffer[4] = { 0 };
uint8_t ADS129X::ADS129X_data[CHANNEL_DATA_SIZE * (NUM_CHANNELS + 1)]  ={ 0 };
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
    "RLD negative"};

const char *gains[]{
    "Gain 6x",
    "Gain 1x",
    "Gain 2x",
    "Gain 3x",
    "Gain 4x",
    "Gain 8x",
    "Gain 12x"};

ADS129X::ADS129X()
{
}

/**
 * Initializes ADS129x library.
 * @param _DRDY data ready pin
 * @param _CS   chip-select pin
 */
void ADS129X::init(int _DRDY, int _CS)
{
    DBG("ADS Setup");
    // initialize the  data ready and chip select pins:
    DRDY = _DRDY;
    ADS129X_CS = _CS;

    DBG("SPI Setup");
    // SPI Setup
    SPI.begin(EMG_SCLK, EMG_MISO, EMG_MOSI, EMG_CS1);

    // set clock divider
    SPI.setClockDivider(SPI_CLOCK_DIV2);

    // set data mode
    SPI.setDataMode(SPI_MODE1); // clock polarity = 0; clock phase = 1 (pg. 8)

    // set bit order
    SPI.setBitOrder(MSBFIRST); // SPI data format is MSB (pg. 25)

    DBG("PIN Setup");
    // clk provided by internal clock
    pinMode(EMG_CLK, INPUT);
    pinMode(EMG_START, OUTPUT);
    // pinMode(DRDY, INPUT_PULLUP);
    pinMode(DRDY, INPUT);
    pinMode(ADS129X_CS, OUTPUT);
    // start triggered by start command
    digitalWrite(EMG_START, LOW);
    // cs high -> ads disabled
    digitalWrite(ADS129X_CS, HIGH);

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        channelEnabled[i] = true;
    }

    isrOn = false;

    continousMode = false;

    DBG("ADS Setup complete");
}

/**
 * Reset Registers to Default Values.
 */
void ADS129X::RESET()
{
    ADS129xCommandLock lock(this, false);

    sendCommand(ADS129X_CMD_RESET);

    delay(10); // must wait 18 tCLK cycles to execute this command (Datasheet, pg. 38)

    // start triggered by start command
    digitalWrite(EMG_START, LOW);
    // cs high -> ads disabled
    digitalWrite(ADS129X_CS, HIGH);

    // stop streaming
    sendCommand(ADS129X_CMD_SDATAC);

    // init variables
    ADS129X_newData = false;
    isrOn = false;
    continousMode = false;

    ADS129X_bufferedTransfer = false;
    
    for (int i = 0; i < CHANNEL_DATA_SIZE * (NUM_CHANNELS + 1); i++)
    {
        ADS129X_data[i] = 0;
    }
}

// System Commands
void ADS129X::sendCommand(byte cmd)
{
    DBG("Sending command[0x%x]", cmd);
    digitalWrite(ADS129X_CS, LOW); // Low to communicate
    SPI.transfer(cmd);
    delayMicroseconds(2);
    digitalWrite(ADS129X_CS, HIGH); // High to end communication
    delayMicroseconds(2);   // must way at least 4 tCLK cycles before sending another command (Datasheet, pg. 38)
}

/**
 * Exit Standby Mode.
 */
void ADS129X::WAKEUP()
{
    ADS129xCommandLock lock(this);

    sendCommand(ADS129X_CMD_WAKEUP);
}

/**
 * Enter Standby Mode.
 */
void ADS129X::STANDBY()
{
    ADS129xCommandLock lock(this);

    sendCommand(ADS129X_CMD_STANDBY);
}

/**
 * Enable Read Data Continuous mode (default).
 */
void ADS129X::RDATAC()
{
    if (isrOn) {
        ERR("Interrupt enabled, use getData() instead");
        return;
    }

    DBG("RDATAC");

    START();
    enableIrq();
    sendCommand(ADS129X_CMD_RDATAC);
}

/**
 * Stop Read Data Continuously mode.
 */
void ADS129X::SDATAC()
{
    DBG("SDATAC");
    // irq is conditionally disabled, always try to disable it
    disableIrq();
    // stop sending data
    STOP();
    // send Stop data continuous command
    sendCommand(ADS129X_CMD_SDATAC);
}

/**
 * Start/restart (synchronize) conversions.
 */
void ADS129X::START()
{
    DBG("Start");

    digitalWrite(EMG_START, HIGH);
    // sendCommand(ADS129X_CMD_START);
}

/**
 * Stop conversion.
 */
void ADS129X::STOP()
{
    DBG("Stop");

    // sendCommand(ADS129X_CMD_STOP);
    digitalWrite(EMG_START, LOW);
}

void ADS129X::enableIrq()
{
    DBG("Attaching interrupt");
    isrOn = true;
    attachInterrupt(DRDY, ADS129X_dataReadyISR, FALLING);
}

void ADS129X::disableIrq()
{
    if (isrOn)
    {
        DBG("Detach interrupt");
        isrOn = false;
        detachInterrupt(DRDY);
    }
}

/**
 * Read data by command; supports multiple read back.
 */
void ADS129X::RDATA(long *buffer)
{
    if (ADS129X_bufferedTransfer)
    {
        ERR("Buffered transfer enabled, use getData() instead");
        return;
    }

    digitalWrite(ADS129X_CS, LOW);
    SPI.transfer(ADS129X_CMD_RDATA);

    // delayMicroseconds(2);

    // always read status
    // read status keeping byte ordering
    ADS129X_data[3] = 0;
    ADS129X_data[2] = SPI.transfer(0x00);
    ADS129X_data[1] = SPI.transfer(0x00);
    ADS129X_data[0] = SPI.transfer(0x00);

    for (int i = 0; i < NUM_CHANNELS; i++)
    {
        long dataPacket = 0;
        for (int j = 0; j < 3; j++)
        {
            byte dataByte = SPI.transfer(0x00);
            dataPacket = (dataPacket << 8) | dataByte;
        }
        if (dataPacket & 0x800000)
            buffer[i] = 0xFF000000 | dataPacket;
        else
            buffer[i] = dataPacket;
    }
    digitalWrite(ADS129X_CS, HIGH);
}

/**
 * Read register at address _address.
 * @param  _address register address
 * @return          value of register
 */
byte ADS129X::RREG(byte _address)
{

    byte opcode1 = ADS129X_CMD_RREG | (_address & 0x1F); // 001rrrrr; _RREG = 00100000 and _address = rrrrr

    ADS129xCommandLock lock(this);

    digitalWrite(ADS129X_CS, LOW);            // Low to communicate
    SPI.transfer(ADS129X_CMD_SDATAC); // SDATAC
    SPI.transfer(opcode1);            // RREG
    SPI.transfer(0x00);               // opcode2
    delayMicroseconds(1);
    byte data = SPI.transfer(0x00); // returned byte should match default of register map unless edited manually (Datasheet, pg.39)
    delayMicroseconds(2);
    digitalWrite(ADS129X_CS, HIGH); // High to end communication

    return data;
}

/**
 * Read _numRegisters register starting at address _address.
 * @param _address      start address
 * @param _numRegisters number of registers
 * @param data          pointer to data array
 */
void ADS129X::RREG(byte _address, byte _numRegisters, byte *_data)
{
    byte opcode1 = ADS129X_CMD_RREG | (_address & 0x1F); // 001rrrrr; _RREG = 00100000 and _address = rrrrr

    ADS129xCommandLock lock(this);

    digitalWrite(ADS129X_CS, LOW);            // Low to communicated
    SPI.transfer(ADS129X_CMD_SDATAC); // SDATAC
    SPI.transfer(opcode1);            // RREG
    SPI.transfer(_numRegisters - 1);  // opcode2
    for (byte i = 0; i < _numRegisters; i++)
    {
        *(_data + i) = SPI.transfer(0x00); // returned byte should match default of register map unless previously edited manually (Datasheet, pg.39)
    }
    delayMicroseconds(2);
    digitalWrite(ADS129X_CS, HIGH); // High to end communication
}

/**
 * Write register at address _address.
 * @param _address register address
 * @param _value   register value
 */
void ADS129X::WREG(byte _address, byte _value)
{
    // DBG("Write reg[%d], value[%d]", _address, _value);
    byte opcode1 = ADS129X_CMD_WREG | (_address & 0x1F); // 001rrrrr; _RREG = 00100000 and _address = rrrrr

    ADS129xCommandLock lock(this);

    digitalWrite(ADS129X_CS, LOW); // Low to communicate
    SPI.transfer(opcode1);
    SPI.transfer(0x00); // opcode2; only write one register
    SPI.transfer(_value);
    delayMicroseconds(2);
    digitalWrite(ADS129X_CS, HIGH); // Low to communicate
}

/**
 * Read device ID.
 * @return device ID
 */
byte ADS129X::getDeviceId()
{
    ADS129xCommandLock lock(this);

    digitalWrite(ADS129X_CS, LOW);          // Low to communicate
    SPI.transfer(ADS129X_CMD_RREG); // RREG
    SPI.transfer(0x00);             // Asking for 1 byte
    byte data = SPI.transfer(0x00); // byte to read (hopefully 0b???11110)
    delayMicroseconds(2);
    digitalWrite(ADS129X_CS, HIGH); // Low to communicate

    DBG("Device Id[%d]", data);

    return data;
}

void ADS129X::setBufferedTransfer(int size, int frame)
{
    if (size > 0)
    {
        DBG("Buffered transfer enabled, size[%d]", size * NUM_CHANNELS);
        ADS129X_bufferedTransfer = true;
        ADS129X_dataBuffer.size(size * NUM_CHANNELS, CHANNEL_DATA_SIZE, frame);
    }
    else
    {
        DBG("Buffered transfer disabled");
        ADS129X_bufferedTransfer = false;
        ADS129X_dataBuffer.size(0, 0, 0);
    }
}

long ADS129X::getStatus()
{
    return (long)ADS129X_data[0];
}

int ADS129X::getTicks()
{
    return isr_tick;
}

/**
 * Interrupt that gets called when DRDY goes HIGH.
 * Transfers data and sets a flag.
 */
void ADS129X_dataReadyISR()
{
    isr_tick++;

    digitalWrite(ADS129X::ADS129X_CS, LOW);

    isr_in++;

    // always read status
    // read status keeping byte ordering
    ADS129X::ADS129X_data[3] = 0;
    ADS129X::ADS129X_data[2] = SPI.transfer(0x00);
    ADS129X::ADS129X_data[1] = SPI.transfer(0x00);
    ADS129X::ADS129X_data[0] = SPI.transfer(0x00);

    if (ADS129X_bufferedTransfer)
    {
        int k = 0;

        for (int i = 0; i < NUM_CHANNELS; i++)
        {
            // always read data
            ADS129X::ADS129X_tempBuffer[2] = SPI.transfer(0x00);
            ADS129X::ADS129X_tempBuffer[1] = SPI.transfer(0x00);
            ADS129X::ADS129X_tempBuffer[0] = SPI.transfer(0x00);

            if (ADS129X::ADS129X_tempBuffer[2] & 0x80) {
                ADS129X::ADS129X_tempBuffer[3] = 0xFF; // Sign extend for two's complement
            } else {
                ADS129X::ADS129X_tempBuffer[3] = 0x00;
            }

            if (channelEnabled[i]) {
                // enqueue in buffer
                ADS129X::ADS129X_data[CHANNEL_STATUS_SIZE + k + 0] = ADS129X::ADS129X_tempBuffer[0];
                ADS129X::ADS129X_data[CHANNEL_STATUS_SIZE + k + 1] = ADS129X::ADS129X_tempBuffer[1];
                ADS129X::ADS129X_data[CHANNEL_STATUS_SIZE + k + 2] = ADS129X::ADS129X_tempBuffer[2];
                ADS129X::ADS129X_data[CHANNEL_STATUS_SIZE + k + 3] = ADS129X::ADS129X_tempBuffer[3];

                k += 4;
            }
        }

        if (k > 0)
            ADS129X::ADS129X_dataBuffer.produce(ADS129X::ADS129X_data + 4, k);
    }
    else
    {
        // channel 1
        ADS129X::ADS129X_data[1 * CHANNEL_DATA_SIZE + 3] = 0;
        ADS129X::ADS129X_data[1 * CHANNEL_DATA_SIZE + 2] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[1 * CHANNEL_DATA_SIZE + 1] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[1 * CHANNEL_DATA_SIZE + 0] = SPI.transfer(0x00);
        // channel 2
        ADS129X::ADS129X_data[2 * CHANNEL_DATA_SIZE + 3] = 0;
        ADS129X::ADS129X_data[2 * CHANNEL_DATA_SIZE + 2] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[2 * CHANNEL_DATA_SIZE + 1] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[2 * CHANNEL_DATA_SIZE + 0] = SPI.transfer(0x00);
        // channel 3
        ADS129X::ADS129X_data[3 * CHANNEL_DATA_SIZE + 3] = 0;
        ADS129X::ADS129X_data[3 * CHANNEL_DATA_SIZE + 2] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[3 * CHANNEL_DATA_SIZE + 1] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[3 * CHANNEL_DATA_SIZE + 0] = SPI.transfer(0x00);
        // channel 4
        ADS129X::ADS129X_data[4 * CHANNEL_DATA_SIZE + 3] = 0;
        ADS129X::ADS129X_data[4 * CHANNEL_DATA_SIZE + 2] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[4 * CHANNEL_DATA_SIZE + 1] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[4 * CHANNEL_DATA_SIZE + 0] = SPI.transfer(0x00);
        // channel 5
        ADS129X::ADS129X_data[5 * CHANNEL_DATA_SIZE + 3] = 0;
        ADS129X::ADS129X_data[5 * CHANNEL_DATA_SIZE + 2] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[5 * CHANNEL_DATA_SIZE + 1] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[5 * CHANNEL_DATA_SIZE + 0] = SPI.transfer(0x00);
        // channel 6
        ADS129X::ADS129X_data[6 * CHANNEL_DATA_SIZE + 3] = 0;
        ADS129X::ADS129X_data[6 * CHANNEL_DATA_SIZE + 2] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[6 * CHANNEL_DATA_SIZE + 1] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[6 * CHANNEL_DATA_SIZE + 0] = SPI.transfer(0x00);
        // channel 7
        ADS129X::ADS129X_data[7 * CHANNEL_DATA_SIZE + 3] = 0;
        ADS129X::ADS129X_data[7 * CHANNEL_DATA_SIZE + 2] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[7 * CHANNEL_DATA_SIZE + 1] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[7 * CHANNEL_DATA_SIZE + 0] = SPI.transfer(0x00);
        // channel 8
        ADS129X::ADS129X_data[8 * CHANNEL_DATA_SIZE + 3] = 0;
        ADS129X::ADS129X_data[8 * CHANNEL_DATA_SIZE + 2] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[8 * CHANNEL_DATA_SIZE + 1] = SPI.transfer(0x00);
        ADS129X::ADS129X_data[8 * CHANNEL_DATA_SIZE + 0] = SPI.transfer(0x00);

        ADS129X_newData = true;
    }

    digitalWrite(ADS129X::ADS129X_CS, HIGH);

    isr_out++;
}

int ADS129X::avail()
{
    return ADS129X_dataBuffer.avail();
}

/**
 * Receive data when in continuous read mode.
 * @param buffer buffer for received data
 * @return true when received data
 */
boolean ADS129X::getData(long *buffer, int timeout)
{
    if (isrOn)
    {
        if (ADS129X_newData)
        {
            ADS129X_newData = false;
            // read only data, skipping status
            for (int i = 0; i < NUM_CHANNELS; i++)
            {
                buffer[i] = (long)(&ADS129X_data[i*CHANNEL_DATA_SIZE + CHANNEL_STATUS_SIZE]);
            }
            return true;
        }
        return false;
    }
    else
    {
        START();

        // wait data ready
        uint32_t start = micros();
        while (digitalRead(DRDY) != LOW)
        {
            uint32_t delta = micros() - start;
            if (delta > timeout)
            {
                ERR("Read timeout expired");
                return false;
            }
        }

        STOP();

        RDATA(buffer);

        return true;
    }
}

/**
 * Configure channel _channel.
 * @param _channel   channel (1-8)
 * @param _powerDown power down (true, false)
 * @param _gain      gain setting
 * @param _mux       mux setting
 */
void ADS129X::configChannel(byte _channel, boolean _powerDown, byte _gain, byte _mux)
{

    channelEnabled[_channel] = !_powerDown;

    if (_powerDown)
    {
        DBG("Forcing channel [%d] mux to input shorted", _channel);
        _mux = ADS129X_MUX_SHORT;
    }

    byte value = ((_powerDown & 1) << 7) | ((_gain & 7) << 4) | (_mux & 7);
    DBG("Channel[%d], Enabled[%s], Source[%s], Gain[%s]", _channel, _powerDown ? "false" : "true", sources[_mux], gains[_gain]);

    WREG(ADS129X_REG_CH1SET + _channel, value);
}

void ADS129X::addEvent(const char *evt)
{
    if (numEvents < MAX_EVENTS)
    {
        events[numEvents++] = evt;
    }
}

void ADS129X::poll()
{
    for (int i = 0; i < numEvents; i++)
    {
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

BufferProducer &ADS129X::getBufferProducer()
{
    return ADS129X_dataBuffer;
}