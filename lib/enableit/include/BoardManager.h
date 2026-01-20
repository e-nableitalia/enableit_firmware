//
// BoardManager: enable.it platform/board implementation
//
// Author: A.Navatta / e-Nable Italia

#ifndef BOARD_MANAGER_H
#define BOARD_MANAGER_H

#include <Arduino.h>
#include <Console.h>
#include <Board.h>
#include <BoardApp.h>

// Timeout in secs for error print in dev mode
#define PANIC_TIMEOUT   20

// Enable to add delay to main loop
//#define LOOP_DELAY      500

namespace enableit {

class BoardManager {
public:
    BoardManager();
    void init();
    
    void loop();

    bool setApp(const char *state_name);

    bool addApp(BoardApp *state);
    bool addAppNoLock(BoardApp *state);
    bool hasApp(const char *state_name);

    void resetApps();

    void panic(int code, const char *description);

    BoardApp **getApps();
    BoardApp *getAppByName(const char *name);
    BoardApp *getCurrentApp();

    Board &getBoard();

    static BoardManager &instance();

private:
    void setApp(BoardApp *state);

    BoardApp *currentApp = nullptr;
    BoardApp *apps[MAX_APPS] = {nullptr};
};

class BoardAppRegistrar {
public:
    BoardAppRegistrar(BoardApp &app) {
        BoardManager::instance().addAppNoLock(&app);
    }
};

#if !defined(NO_GLOBAL_INSTANCES)
extern BoardManager eBoard;
#endif

} // namespace enableit

// Platform management and app registration macros

#define DECLARE_BOARDAPP(APP_TYPE)    \
    extern "C" ::enableit::BoardApp&                 \
        get_##APP_TYPE##_app();                      

#define ENABLE_BOARD_APP(APP_TYPE)                   \
    extern "C" ::enableit::BoardApp&                 \
        get_##APP_TYPE##_app();                      \
    static ::enableit::BoardAppRegistrar             \
        _registrar_##APP_TYPE(get_##APP_TYPE##_app());

#define REGISTER_BOARD_APP(APP_TYPE)    \
        ::enableit::eBoard.addApp(      \
            &get_##APP_TYPE##_app());
    
#define ENABLEIT_BOOT(APP) do { \
    enableit::eBoard.init(); \
    enableit::eBoard.setApp(APP); \
} while(0)

#define ENABLEIT_LOOP() enableit::eBoard.loop()

#endif // BOARD_MANAGER_H