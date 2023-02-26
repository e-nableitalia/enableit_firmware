#include <Arduino.h>
#include <mutex>
#include <BoardManager.h>
#include <Config.h>
#include <debug.h>

std::recursive_mutex state_mtx;

#if !defined(NO_GLOBAL_INSTANCES)
BoardManager eBoard;
#endif

BoardManager::BoardManager() {
    // init serial with default baud rate
    //Serial.begin(115200);
    currentApp = nullptr;
}

void BoardManager::init() {

    debug_enable(true);
    DBG("Init");

    config.init();

//    if (config.baudRate != 0) {
//        Serial.updateBaudRate(config.baudRate);
//    }

    for (int i = 0; i < MAX_APPS; i++)
        apps[i] = nullptr;
    
    DBG("Init done");
}

bool BoardManager::addApp(BoardApp *state) {
    if (state) {
        std::lock_guard<std::recursive_mutex> lck(state_mtx);
        for (int i = 0; i < MAX_APPS; i++)
            if (apps[i] == nullptr) {
                DBG("Adding state[%s] in position[%d]", state->name(), i);
                apps[i] = state;
                return true;
            }
    }
    DBG("Failed to add state[%s]", state->name());
    return false;
}

bool BoardManager::setApp(const char *state) {
    std::lock_guard<std::recursive_mutex> lck(state_mtx);
    for (int i = 0; i < MAX_APPS; i++)
        if ((apps[i] != nullptr) && (!strcmp(state,apps[i]->name()))) {
            if (currentApp != nullptr) {
                DBG("Leaving state[%s]", currentApp->name());
                currentApp->leave();
            }
            currentApp = apps[i];
            DBG("Entering state[%s]", currentApp->name());
            currentApp->enter();
            return true;
        }
    DBG("State[%s] not found", state);
    panic(ENOENT, "State not found");
    return false;
}

void BoardApp::changeApp(const char *state) {
    DBG("Going in state[%s]", state);
    eBoard.setApp(state);
}

void BoardManager::loop() {
    //DBG("board loop");
    {
        std::lock_guard<std::recursive_mutex> lck(state_mtx);
        if (currentApp != nullptr)
            currentApp->process();
    }
#ifdef LOOP_DELAY
    //DBG("Sleeping[%d] milli seconds", LOOP_DELAY);
    delay(LOOP_DELAY);
#endif
}

void BoardManager::setApp(BoardApp *s) {
    std::lock_guard<std::recursive_mutex> lck(state_mtx);

    if (currentApp != nullptr)
        currentApp->leave();

    currentApp = s;

    if (currentApp != nullptr)
        currentApp->enter();
}

void BoardManager::panic(int code, const char *description) {
    if (config.devMode) {
        while (1) {
            ERR("Board Error[%d] Reason: %s", code, description);
            sleep(3);
        }
    } else {
        ERR("Board Error[%d] Reason: %s, rebooting in %d seconds", code, description, PANIC_TIMEOUT);
        sleep(PANIC_TIMEOUT);
        ESP.restart();
    }
}