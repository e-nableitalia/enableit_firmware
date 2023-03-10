#include <EMGApp.h>

void EMGApp::enter() {
    DBG("enter EMGApp");
    parser.init(this);
    parser.add("setup", CMD_HELP, &EMGApp::cmdSetup);
    parser.add("data", CMD_INFO, &BootLoader::cmdReadData);
    parser.add("write", CMD_INFO, &BootLoader::cmdWrite);
    parser.add("read", CMD_INFO, &BootLoader::cmdRead);
}

void EMGApp::leave() {
    DBG("leave EMGApp");
}

void EMGApp::process() {
}

void EMGApp::cmdSetup() {
    DBG("EMG Setup");
    emg.setupADS();
}

void EMGApp::cmdReadData() {
    DBG("EMG Read data");

    if (emg.hasData(buffer))
        DBG("Data found");
    else    
        DBG("no data available");
}

void EMGApp::cmdWrite() {
    DBG("EMG Write");

    String register = parser.getString(1);
    String value = parser.getString(2);

    emg.write(register.toInt(), value.toInt());
}

void EMGApp::cmdRead() {
    DBG("EMG Read");
    String register = parser.getString(1);
    emg.read(register.toInt(););
}
