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

#include <SerialEmg.h>

class EMGApp : public BoardApp {
    void enter();
    void leave();
    void process();

    void cmdSetup();
    void cmdReadData();
    void cmdWrite();
    void cmdRead();

    const char *name() { return "emg"; }
    
    SerialEmg   emg;
    long        buffer[9];
};

#endif // REBOOTSTATE_H