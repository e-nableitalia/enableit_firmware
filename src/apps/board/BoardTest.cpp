#include <Arduino.h>
#include "BoardTest.h"
#include <Wire.h>
#include <Adafruit_INA219.h>

#define CMD_TEST "* test, run a test command"
#define CMD_HELP "* help, list available commands"
#define CMD_READ_G1 "* readG1, read analog value from G1"
#define CMD_READ_G2 "* readG2, read analog value from G2"
#define CMD_SETUP_INA219 "* setupINA219, attempt to configure INA219"
#define CMD_POLLING "* polling, toggle polling mode"

Adafruit_INA219 ina219;

#define I2C_SDA GPIO_NUM_38 // SDA pin
#define I2C_SCL GPIO_NUM_39 // SCL pin
#define ANALOG_PIN_G1 GPIO_NUM_1 // Analog input pin G1
#define ANALOG_PIN_G2 GPIO_NUM_2 // Analog input pin G2

bool pollingMode = false;
unsigned long lastPollTime = 0;

void BoardTest::enter() {
    log_d("enter BoardTest");
    parser.init(this);
    parser.add("test", CMD_TEST, &BoardTest::cmdTest);
    parser.add("help", CMD_HELP, &BoardTest::cmdHelp);
    parser.add("readG1", CMD_READ_G1, &BoardTest::cmdReadG1);
    parser.add("readG2", CMD_READ_G2, &BoardTest::cmdReadG2);
    parser.add("setupINA219", CMD_SETUP_INA219, &BoardTest::cmdSetupINA219);
    parser.add("polling", CMD_POLLING, &BoardTest::cmdPolling);
    // Call INA219 setup
    //setupINA219();
    // Configure G1 and G2 as analog inputs
    //pinMode(ANALOG_PIN_G1, INPUT);
    //pinMode(ANALOG_PIN_G2, INPUT);
    analogSetPinAttenuation(ANALOG_PIN_G1, ADC_11db); // permette lettura 0 – ~3.3V
    analogSetPinAttenuation(ANALOG_PIN_G2, ADC_11db); // permette lettura 0 – ~3.3V

    // ...initialize other commands if needed...
}

void BoardTest::leave() {
    log_d("leave BoardTest");
    // ...cleanup if needed...
}

void BoardTest::process() {
    parser.poll();

    if (pollingMode) {
        unsigned long currentTime = millis();
        if (currentTime - lastPollTime >= 1000) {
            lastPollTime = currentTime;

            int rawValueD1 = analogReadMilliVolts(ANALOG_PIN_G1);
            int rawValueD2 = analogReadMilliVolts(ANALOG_PIN_G2);
            double current_mA = ina219.getCurrent_mA();

            log_d("D1: %d mV\tD2: %d mV\tCurrent: %f mA", rawValueD1, rawValueD2, current_mA);
        }
    }
}

void BoardTest::cmdTest() {
    log_d("Test command executed");
    // Example: Read voltage and current from INA219
    double busVoltage = ina219.getBusVoltage_V();
    double shuntVoltage = ina219.getShuntVoltage_mV();
    double current = ina219.getCurrent_mA();
    double power = ina219.getPower_mW();
    log_d("Bus Voltage: %f V", busVoltage);
    log_d("Shunt Voltage: %f mV", shuntVoltage);
    log_d("Current: %f mA", current);
    log_d("Power: %f mW", power);
    // You can add more test logic here if needed
}

void BoardTest::cmdHelp() {
    log_d("Available commands:");
    log_d(CMD_TEST);
    log_d(CMD_HELP);
    log_d(CMD_READ_G1);
    log_d(CMD_READ_G2);
    log_d(CMD_SETUP_INA219);
    log_d(CMD_POLLING); // Add polling command info
}

void BoardTest::cmdReadG1() {
    int value = analogRead(ANALOG_PIN_G1);
    log_d("Analog value from G1: %d", value);
}

void BoardTest::cmdReadG2() {
    int value = analogRead(ANALOG_PIN_G2);
    log_d("Analog value from G2: %d", value);
}

void BoardTest::cmdSetupINA219() {
    log_d("Attempting to configure INA219...");
    for (int attempt = 1; attempt <= 10; ++attempt) {
        log_d("Attempt %d to initialize INA219", attempt);
        if (ina219.begin()) {
            log_d("INA219 initialized successfully on attempt %d", attempt);
            return;
        }
        delay(100); // Wait before retrying
    }
    log_d("Failed to initialize INA219 after 10 attempts");
}

void BoardTest::setupINA219() {
    log_d("Setting up INA219 on I2C pins G38 (SDA) and G39 (SCL)");
    Wire.begin(I2C_SDA, I2C_SCL); // Initialize I2C on G38 (SDA) and G39 (SCL)
    cmdSetupINA219(); // Use the new command to attempt initialization
}

void BoardTest::cmdPolling() {
    pollingMode = !pollingMode;
    if (pollingMode) {
        log_d("Polling mode enabled");
        lastPollTime = millis();
    } else {
        log_d("Polling mode disabled");
    }
}
