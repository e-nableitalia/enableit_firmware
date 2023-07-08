//
// Reboot App
// Simple app performing a countdown and then rebooting the board
//
// Author: A.Navatta / e-Nable Italia

#ifndef EMG_APP_H

#define EMG_APP_H

#include <Arduino.h>
#include <debug.h>
#include <BoardApp.h>

#include <CommandParser.h>

#include <SerialEmg.h>

#include <RTPPackets.h>

class EMGApp : public BoardApp {
    void enter();
    void leave();
    void process();

    void cmdSetup();
    void cmdDestHost();
    void cmdReadData();
    void cmdWrite();
    void cmdRead();
    void cmdSignalHigh();
    void cmdSignalLow();
    void cmdReboot();
    void cmdUpdate();
    void cmdHelp();
    void cmdTest();
    void cmdSource();
    void cmdGain();
    void cmdEnable();
    void cmdDisable();
    void cmdMode();
    void cmdStatus();
    void cmdBuffer();
    void cmdSequence();
    void cmdOffset();

    void sendHost(String _host,int remotePort, int frame, int delay);

    const char *name() { return "emg"; }
    
    SerialEmg   emg;
    long        buffer[9];
    double      offset;
    bool        polling;
    bool        streaming;
    int         delay;
    RTPPacket   packet;
    CommandParser<EMGApp> parser;
};

#endif // REBOOTSTATE_H