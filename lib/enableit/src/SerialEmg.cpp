/*
 * Copyright (C) 2016 - PSYONIC Inc.
 * Author: Aadeel Akhtar <aakhta3@illinois.edu>
 * Author: Edward Wu <elwu2@illinois.edu>
 * Author: Alvin Wu <alvinwu2@illinois.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <Console.h>

#include "SerialEmg.h"  



SerialEmg::SerialEmg() {  
}

/* Setup the registers on the ADS1298 */
void SerialEmg::init() {
  DBG("Data ready pin: %d", EMG_DRDY);
  DBG("CS pin: %d", EMG_CS1);

  ADS.init(EMG_DRDY, EMG_CS1);

  //ADS = ADS129X(EMG_DRDY, EMG_CS1);
  //pinMode(ADS_RESET, OUTPUT);
  //digitalWrite(ADS_RESET, HIGH);
  delay(100); // delay for power-on-reset (Datasheet, pg. 48)
  // reset pulse
  //digitalWrite(ADS_RESET, LOW);
  //digitalWrite(ADS_RESET, HIGH);
  ADS.RESET();

  delay(1); // Wait for 18 tCLKs AKA 9 microseconds, we use 1 millisec

  ADS.SDATAC(); // device wakes up in RDATAC mode, so send stop signal

  active = false;
  
  byte value = ADS.RREG(ADS129X_REG_ID);
  DBG("ADS129X_REG_ID[0x%x]", value);
  switch (value) {
    case ADS129X_ID_ADS1294:
        DBG("ChipId: ADS1294, enabling 4 channel");
        maxChannels = 4;
        break;
    case ADS129X_ID_ADS1296:
        DBG("ChipId: ADS1296, enabling 6 channel");
        maxChannels = 6;
        break;    
    case ADS129X_ID_ADS1298:
        DBG("ChipId: ADS1298, enabling 8 channel");
        maxChannels = 8;
        break;    
    case ADS129X_ID_ADS1294R:
        DBG("ChipId: ADS1294R, enabling 4 channel");
        maxChannels = 4;
        break;
    case ADS129X_ID_ADS1296R:
        DBG("ChipId: ADS1296R, enabling 6 channel");
        maxChannels = 6;
        break;    
    case ADS129X_ID_ADS1298R:
        DBG("ChipId: ADS1298R, enabling 8 channel");
        maxChannels = 8;
        break;
    default:
        DBG("ChipId: Unknown(%x)", value);            
  }

  setRate(RATE_2K);

  bufferDepth = 2000; // 1 second buffer, default

  ADS.WREG(ADS129X_REG_CONFIG3, (1<<ADS129X_BIT_PD_REFBUF) | (1<<6) | /* RLDRF is 2.4V */ /* (1<<ADS129X_BIT_RLD_MEAS) | */ (1<<ADS129X_BIT_RLDREF_INT)   | (1<<ADS129X_BIT_PD_RLD)); // enable internal reference
  value = ADS.RREG(ADS129X_REG_CONFIG3);
  DBG("ADS129X_REG_CONFIG3[0x%x]", value);

  // all channels disabled
  state = 0;

  for (int i = 0; i < maxChannels; i++) {
    // set channel to default settings
    DBG("Configuring channel[%d]", i);
    setSrc(i,SRC_ELECTRODE);
    setGain(i, GAIN_6X);
    enable(i);
    //ADS.configChannel(i, false, ADS129X_GAIN_1X, ADS129X_MUX_TEST); //ADS129X_MUX_NORMAL);
    //buffer[i] = 0;
  }
}

void SerialEmg::fini() {
  DBG("Finalizing");
  ADS.SDATAC();
}

void SerialEmg::streaming(bool on) {
  if (on) { 
    int buffer_size = 0;
    switch (rate) {
      case RATE_250:
          buffer_size = 250 * bufferDepth / 1000;
          break;
      case RATE_500:
          buffer_size = 500 * bufferDepth / 1000;
          break;
      case RATE_1K:
          buffer_size = bufferDepth; // 1000 * depth / 1000
          break;
      default:
      case RATE_2K:
          buffer_size = 2 * bufferDepth; // 2000 * depth / 1000
          break;
      case RATE_4K:
          buffer_size = 4 * bufferDepth; // 4000 * depth / 1000
          break;
      case RATE_8K:
          buffer_size = 8 * bufferDepth; // 8000 * depth / 1000
          break;
      case RATE_16K:
          buffer_size = 16 * bufferDepth; // 16000 * depth / 1000
          break;
    }
    DBG("Buffer depth[%d], buffer size[%d]", bufferDepth, buffer_size);
    delay(1);
    ADS.setBufferedTransfer(buffer_size);
    ADS.RDATAC();
    delay(1);    
  } else {
    DBG("Deactivate streaming");
    delay(1);
    ADS.SDATAC(); // device wakes up in RDATAC mode, so send stop signal
    ADS.setBufferedTransfer(0);
    delay(1);
  }
}

void SerialEmg::setTestMode(bool enable_2x, TEST_SIGNAL signal) {

  testSignal = signal;

  byte config = (1<<ADS129X_BIT_INT_TEST); // internal test signal
  if (enable_2x) {
    DBG("Test Signal Amplitude: 2 x -(VREFP - VREFN) / 2400 V");
    config |= (1<<ADS129X_BIT_TEST_AMP);
  } else {
    DBG("Test Signal Amplitude: 1 x -(VREFP - VREFN) / 2400 V");
  }
  switch (signal) {
      case SIG_1HZ:
        config |= ADS129X_TEST_FREQ_1HZ;
        DBG("Test Signal Frequency: 1Hz");
        break; 
      case SIG_DC:     
        config |= ADS129X_TEST_FREQ_DC;
        DBG("Test Signal Frequency: DC ref");
        break;        
      default:
      case SIG_2HZ:
        config |= ADS129X_TEST_FREQ_2HZ;
        DBG("Test Signal Frequency: 2Hz");
        break;
  }

  ADS.WREG(ADS129X_REG_CONFIG2, config);
  byte value = ADS.RREG(ADS129X_REG_CONFIG2);
  DBG("ADS129X_REG_CONFIG2[0x%x]", value);
}

void SerialEmg::setRate(DATA_RATE r) {
    byte rr;
    
    rate = r;

    switch (r) {
        case RATE_16K:
          rr = ADS129X_SAMPLERATE_16;
          DBG("Set DataRate[16K SPS]");
          break;
        case RATE_8K:
          rr = ADS129X_SAMPLERATE_32;
          DBG("Set DataRate[8K SPS]");
          break;
        case RATE_4K:
          rr = ADS129X_SAMPLERATE_64;
          DBG("Set DataRate[4K SPS]");
          break;
        case RATE_2K:
          rr = ADS129X_SAMPLERATE_128;
          DBG("Set DataRate[2K SPS]");
          break;
        default:
        case RATE_1K:
          rr = ADS129X_SAMPLERATE_256;
          DBG("Set DataRate[1K SPS]");
          break;
        case RATE_500:
          rr = ADS129X_SAMPLERATE_512;
          DBG("Set DataRate[512 SPS]");
          break;
        case RATE_250:
          rr = ADS129X_SAMPLERATE_1024;
          DBG("Set DataRate[250 SPS]");
          break;
    }

    ADS.WREG(ADS129X_REG_CONFIG1,(1<<ADS129X_BIT_CLK_EN) | rr); //ADS129X_SAMPLERATE_256); // enable 8kHz sample-rate (p.67)
    byte value = ADS.RREG(ADS129X_REG_CONFIG1);
    DBG("ADS129X_REG_CONFIG1[0x%x]", value);
}

void SerialEmg::setGain(int channel, CHANNEL_GAIN g) {  
 if (channel < maxChannels) {
    byte gain = 0;
    switch (g) {
      case GAIN_1X:
        gain = ADS129X_GAIN_1X;
        DBG("Channel[%d], Gain[1X]", channel);
        break;
      case GAIN_2X:
        gain = ADS129X_GAIN_2X;
        DBG("Channel[%d], Gain[2X]", channel);
        break;
      case GAIN_3X:
        gain = ADS129X_GAIN_3X;
        DBG("Channel[%d], Gain[3X]", channel);
        break;        
      case GAIN_4X:
        gain = ADS129X_GAIN_4X;
        DBG("Channel[%d], Gain[4X]", channel);
        break;
      case GAIN_8X:
        gain = ADS129X_GAIN_8X;
        DBG("Channel[%d], Gain[8X]", channel);
        break;
      case GAIN_12X:
        gain = ADS129X_GAIN_12X;
        DBG("Channel[%d], Gain[12X]", channel);
        break;
      default:
      case GAIN_6X:
        gain = ADS129X_GAIN_6X;
        DBG("Channel[%d], Gain[6X]", channel);
        break;
    }  

    byte value = ADS.RREG(ADS129X_REG_CH1SET + channel);
    byte newvalue = value & 0x8F | ((gain & 7)<<4);
    
    ADS.WREG(ADS129X_REG_CH1SET + channel, newvalue);
  } else {
    ERR("Channel[%d] not supported", channel);
  }
}

void SerialEmg::setSrc(int channel, CHANNEL_SRC src) {
  if (channel < maxChannels) {
    byte source = 0;
    switch (src) {
        case SRC_NOISE:
          source = ADS129X_MUX_SHORT;
          DBG("Channel[%d], Source[noise]", channel);
        break;
        case SRC_SUPPLY:
          source = ADS129X_MUX_MVDD;
          DBG("Channel[%d], Source[supply]", channel);
        break;                
        case SRC_TEMPERATURE:
          source = ADS129X_MUX_TEMP;
          DBG("Channel[%d], Source[temperature]", channel);
        break;
        case SRC_TEST:
          source = ADS129X_MUX_TEST;
          DBG("Channel[%d], Source[test]", channel);
        break;
        case SRC_RLD_MEAS:
          source = ADS129X_MUX_RLD_MEAS;
          DBG("Channel[%d], Source[rld meas]", channel);
        break;                        
        case SRC_RLD_POS:
          source = ADS129X_MUX_RLD_DRP;
          DBG("Channel[%d], Source[rld positive]", channel);
        break; 
        case SRC_RLD_NEG:
          source = ADS129X_MUX_RLD_DRN;
          DBG("Channel[%d], Source[rld negative]", channel);
        break;
        default:
        case SRC_ELECTRODE:
          DBG("Channel[%d], Source[electrode]", channel);
          source = ADS129X_MUX_NORMAL;
        break;        
    }  

    byte value = ADS.RREG(ADS129X_REG_CH1SET + channel);
    byte newvalue = value & 0xF8 | (source & 7);

    ADS.WREG(ADS129X_REG_CH1SET + channel, newvalue);
  } else {
    ERR("Channel[%d] not supported", channel);
  }
}

void SerialEmg::enable(int channel) {
  byte value = ADS.RREG(ADS129X_REG_CH1SET + channel);
  DBG("Enabling Channel[%d]", channel);
  byte newvalue = value & 0x7F; // clear first bit

  state |= 1 << channel;
    
  ADS.WREG(ADS129X_REG_CH1SET + channel, newvalue);
}

void SerialEmg::disable(int channel) {
  byte value = ADS.RREG(ADS129X_REG_CH1SET + channel);
  DBG("Disabling Channel[%d]", channel);
  byte newvalue = value | 0x80; // set first bit

  state &= ~(1 << channel);
    
  ADS.WREG(ADS129X_REG_CH1SET + channel, newvalue);
}

void SerialEmg::setBuffer(int size) {
  DBG("Setting data buffer[%d]", size);
  bufferDepth = size;
}

/* Check if there is data available, if so copy it to data */
bool SerialEmg::readData(long *data, int timeout) {
  uint32_t start = micros();
  while (!ADS.getData(data)) {
    uint32_t delta = micros() - start;
    if (delta > timeout) {
      ERR("Read timeout expired");
      return false;
    }

  }
  return true;
}

int SerialEmg::readData(uint8_t *buffer, int size, bool single_frame) {
  BufferProducer *bp = ADS.getBufferProducer();

  if (bp) {
    int available_data = bp->avail();
    if ((single_frame) && (available_data < size)) {
      return 0;
    }
    int chunk = min(size, available_data);
    
    bp->consume(buffer, chunk);
    
  }
  return 0;
}

int SerialEmg::avail() {
  return ADS.avail();
}

void SerialEmg::write(byte reg, byte value) {
  OUT("Register[%d], writing[%d]", reg, value);
  ADS.WREG(reg,value);
}

byte SerialEmg::read(byte reg) {
  byte value = ADS.RREG(reg);
  OUT("Register[%d], read[%x]", reg, value);
  return value;
}
