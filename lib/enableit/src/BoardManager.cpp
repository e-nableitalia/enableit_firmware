#include <Arduino.h>
#include <mutex>
#include <BoardManager.h>
#include <Config.h>
#include <Console.h>

#include <NoopApp.h>
#include <RebootApp.h>

namespace enableit {

extern "C" {    
    extern const BoardAppRegistrar __start_board_apps[];
    extern const BoardAppRegistrar __stop_board_apps[];    
}

std::recursive_mutex state_mtx;

#if !defined(NO_GLOBAL_INSTANCES)
BoardManager eBoard;

NoopApp       noop;
RebootState   reboot;
#endif

BoardManager::BoardManager() {
    currentApp = nullptr;
}

void BoardManager::init() {
    log_i("BoardManager Init");
    Console.init(115200,true);
}

bool BoardManager::addApp(BoardApp *state) {
    std::lock_guard<std::recursive_mutex> lck(state_mtx);
    return addAppNoLock(state);
}

bool BoardManager::addAppNoLock(BoardApp *state) {
    if (state) {
        for (int i = 0; i < MAX_APPS; i++)
            if (apps[i] == nullptr) {
                log_i("Adding App[%s] in position[%d]", state->name(), i);
                apps[i] = state;
                return true;
            }
    }
    log_e("Failed to add App[%s]", state->name());
    return false;
}

bool BoardManager::setApp(const char *state) {
    std::lock_guard<std::recursive_mutex> lck(state_mtx);
    for (int i = 0; i < MAX_APPS; i++)
        if ((apps[i] != nullptr) && (!strcmp(state,apps[i]->name()))) {
            if (currentApp != nullptr) {
                log_d("Leaving App[%s]", currentApp->name());
                currentApp->leave();
            }
            currentApp = apps[i];
            log_d("Entering App[%s]", currentApp->name());
            currentApp->enter();
            return true;
        }
    log_e("App[%s] not found", state);
    panic(ENOENT, "App not found");
    return false;
}

void BoardApp::changeApp(const char *state) {
    log_d("Going in App[%s]", state);
    eBoard.setApp(state);
}

BoardApp **BoardApp::apps() {
    return eBoard.getApps();
}

bool BoardApp::hasApp(const char *state_name) {
    return eBoard.hasApp(state_name);
}

bool BoardManager::hasApp(const char *state_name) {
    std::lock_guard<std::recursive_mutex> lck(state_mtx);
    for (int i = 0; i < MAX_APPS; i++)
        if ((apps[i] != nullptr) && (!strcmp(state_name,apps[i]->name()))) {
            return true;
        }
    return false;
}

void BoardManager::loop() {
    //log_d("board loop");
    {
        std::lock_guard<std::recursive_mutex> lck(state_mtx);
        if (currentApp != nullptr)
            currentApp->process();
    }
#ifdef LOOP_DELAY
    //log_d("Sleeping[%d] milli seconds", LOOP_DELAY);
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
            log_e("Board Error[%d] Reason: %s", code, description);
            sleep(3);
        }
    } else {
        log_e("Board Error[%d] Reason: %s, rebooting in %d seconds", code, description, PANIC_TIMEOUT);
        sleep(PANIC_TIMEOUT);
        ESP.restart();
    }
}

BoardApp *BoardManager::getCurrentApp() {
    return currentApp;
}

BoardApp **BoardManager::getApps() {
    return apps;
}

Board &BoardManager::getBoard() {
    return board;
}

BoardManager &BoardManager::instance() {
    return eBoard;
}

} // namespace enableit