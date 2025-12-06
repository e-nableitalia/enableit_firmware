#ifndef BOARD_TEST_H
#define BOARD_TEST_H

#include <BoardApp.h>
#include <CommandParser.h>

#define BOARD_TEST "boardtest"

class BoardTest : public BoardApp {
public:
    void enter();
    void leave();
    void process();

    const char *name() { return BOARD_TEST; }

private:
    void cmdTest();
    void cmdHelp();
    void cmdReadG1();
    void cmdReadG2();
    void cmdSetupINA219(); // New command for INA219 setup
    void setupINA219(); // Function to set up INA219
    void cmdPolling(); // New command for polling mode

    CommandParser<BoardTest> parser;
};

#endif
