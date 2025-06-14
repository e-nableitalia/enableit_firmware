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

#define I2C_SDA G38 // SDA pin
#define I2C_SCL G39 // SCL pin
#define ANALOG_PIN_G1 G1 // Analog input pin G1
#define ANALOG_PIN_G2 G2 // Analog input pin G2

bool pollingMode = false;
unsigned long lastPollTime = 0;
bool testExecuted = false; // Track if cmdTest has been executed at least once

void BoardTest::enter() {
    DBG("enter BoardTest");
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
    DBG("leave BoardTest");
    // ...cleanup if needed...
}

void BoardTest::process() {
    if (parser.poll()) {
        if (pollingMode && testExecuted) { // Disable only after at least one test execution
            DBG("Polling mode disabled");
            pollingMode = false;
        }
    }

    if (pollingMode) {
        unsigned long currentTime = millis();
        if (currentTime - lastPollTime >= 1000 || !testExecuted) { // Execute immediately if not yet executed
            lastPollTime = currentTime;

            // Read raw analog values
            int rawValueD1 = analogReadMilliVolts(ANALOG_PIN_G1);
            int rawValueD2 = analogReadMilliVolts(ANALOG_PIN_G2);

            // // Normalize to real voltage (0-5V)
            // double voltageD1 = (rawValueD1 / 4095.0) * 5.0; // Assuming 12-bit ADC (0-4095)
            // double voltageD2 = (rawValueD2 / 4095.0) * 5.0;

            // Read current in mA from INA219
            double current_mA = ina219.getCurrent_mA();

            // Print normalized voltages and current in a single line
            DBG("D1: %d mV\tD2: %d mV\tCurrent: %f mA", rawValueD1, rawValueD2, current_mA);

            testExecuted = true; // Mark that at least one execution has occurred
        }
    }
    parser.poll();
    // ...process logic if needed...
}

void BoardTest::cmdTest() {
    DBG("Test command executed");
    // Example: Read voltage and current from INA219
    double busVoltage = ina219.getBusVoltage_V();
    double shuntVoltage = ina219.getShuntVoltage_mV();
    double current = ina219.getCurrent_mA();
    double power = ina219.getPower_mW();
    DBG("Bus Voltage: %f V", busVoltage);
    DBG("Shunt Voltage: %f mV", shuntVoltage);
    DBG("Current: %f mA", current);
    DBG("Power: %f mW", power);
    // You can add more test logic here if needed
}

void BoardTest::cmdHelp() {
    DBG("Available commands:");
    DBG(CMD_TEST);
    DBG(CMD_HELP);
    DBG(CMD_READ_G1);
    DBG(CMD_READ_G2);
    DBG(CMD_SETUP_INA219);
    DBG(CMD_POLLING); // Add polling command info
}

void BoardTest::cmdReadG1() {
    int value = analogRead(ANALOG_PIN_G1);
    DBG("Analog value from G1: %d", value);
}

void BoardTest::cmdReadG2() {
    int value = analogRead(ANALOG_PIN_G2);
    DBG("Analog value from G2: %d", value);
}

void BoardTest::cmdSetupINA219() {
    DBG("Attempting to configure INA219...");
    for (int attempt = 1; attempt <= 10; ++attempt) {
        DBG("Attempt %d to initialize INA219", attempt);
        if (ina219.begin()) {
            DBG("INA219 initialized successfully on attempt %d", attempt);
            return;
        }
        delay(100); // Wait before retrying
    }
    DBG("Failed to initialize INA219 after 10 attempts");
}

void BoardTest::setupINA219() {
    DBG("Setting up INA219 on I2C pins G38 (SDA) and G39 (SCL)");
    Wire.begin(I2C_SDA, I2C_SCL); // Initialize I2C on G38 (SDA) and G39 (SCL)
    cmdSetupINA219(); // Use the new command to attempt initialization
}

void BoardTest::cmdPolling() {
    pollingMode = !pollingMode;
    if (pollingMode) {
        DBG("Polling mode enabled");
        lastPollTime = millis(); // Reset the timer
        testExecuted = false; // Reset the test execution flag
    } else {
        DBG("Polling mode disabled");
    }
}
