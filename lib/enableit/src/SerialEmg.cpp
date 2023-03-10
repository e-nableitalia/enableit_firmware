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
#include <debug.h>

#include "SerialEmg.h"  

SerialEmg::SerialEmg() {  
}

/* Setup the registers on the ADS1298 */
void SerialEmg::setupADS() {
  ADS = ADS129X(ADS_DRDY, ADS_CS);

  DBG("Reset pin: %d\n", ADS_RESET);
  DBG("Data ready pin: %d\n", ADS_DRDY);
  DBG("CS pin: %d\n", ADS_CS);

  //pinMode(ADS_RESET, OUTPUT);

  //digitalWrite(ADS_RESET, HIGH);
  delay(100); // delay for power-on-reset (Datasheet, pg. 48)
  // reset pulse
  //digitalWrite(ADS_RESET, LOW);
  //digitalWrite(ADS_RESET, HIGH);
  ADS.RESET();

  delay(1); // Wait for 18 tCLKs AKA 9 microseconds, we use 1 millisec

  ADS.SDATAC(); // device wakes up in RDATAC mode, so send stop signal
  
  byte value = ADS.RREG(ADS129X_REG_ID);
  DBG("Read value[%d]", value);

  ADS.WREG(ADS129X_REG_CONFIG1,(1<<ADS129X_BIT_CLK_EN) | ADS129X_SAMPLERATE_256); // enable 8kHz sample-rate (p.67)
  value = ADS.RREG(ADS129X_REG_CONFIG1);
  DBG("Read value[%d]", value);

  ADS.WREG(ADS129X_REG_CONFIG3, (1<<ADS129X_BIT_PD_REFBUF) | (1<<6) | (1<<ADS129X_BIT_RLD_MEAS) | (1<<ADS129X_BIT_RLDREF_INT) | (1<<ADS129X_BIT_PD_RLD)); // enable internal reference
    value = ADS.RREG(ADS129X_REG_CONFIG3);
  DBG("Read value[%d]", value);

  ADS.WREG(ADS129X_REG_CONFIG2, (1<<ADS129X_BIT_INT_TEST | ADS129X_TEST_FREQ_2HZ));
  value = ADS.RREG(ADS129X_REG_CONFIG2);
  DBG("Read value[%d]", value);

  for (int i = 1; i <= NUM_CHANNELS; i++) {
    ADS.configChannel(i, false, ADS129X_GAIN_12X, ADS129X_MUX_NORMAL);
  }

  delay(1);
  ADS.RDATAC();
  ADS.START();
}

/* Check if there is data available, if so copy it to data */
bool SerialEmg::hasData(long* data) {
  return ADS.getData(data);
}

void SerialEmg::write(byte reg, byte value) {
  OUT("Register[%d], writing[%d]", reg, value);
  ADS.WREG(reg,value);
}

void SerialEmg::read(byte reg) {
  byte value = ADS.RREG(reg);
  OUT("Register[%d], read[%d]", reg, value);
}
