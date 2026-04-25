//
// BoardApp class definition (abstract board application)
//
// Author: A.Navatta / e-Nable Italia

#pragma once

#include <functional> // aggiungi questo include per std::function
#include <Arduino.h>

// Max allowed applications
#define MAX_APPS      16

// default known applications
#define APP_NOOP      "noop"
#define APP_BOOT      "boot"
#define APP_OTAUPDATE "otaupdate"
#define APP_OTAWEBUPDATE "otaweb"

// special run states
#define STATE_INIT      "init"
#define STATE_RUNNING   "run"
#define STATE_REBOOT    "reboot"

namespace enableit {
    namespace capabilities {
        constexpr const char* Notify = "notify";
    }
class BoardApp {
public:
    using NotifyFn = std::function<void(const String&)>;

    BoardApp() = default;
    virtual ~BoardApp() = default;

    virtual void enter() = 0;
    virtual void process() = 0;
    virtual void leave() = 0;
    virtual const char *name() = 0;
    void changeApp(const char *name);
    BoardApp **apps();
    bool hasApp(const char *state_name);
    virtual String getInfo(String key) const {
        return "";
    }
    virtual void setInfo(String key, String value) {
        // default: do nothing
    }
    // Capability query (override in derived classes), useful for features detection without dynamic_cast
    virtual bool hasCapability(const char* cap) const {
        return false;
    }
    virtual void setNotifyFn(NotifyFn fn) {
        // default: do nothing
    }
};

} // namespace enableit

#define BOARDAPP_INSTANCE(APP_TYPE)                 \
    static APP_TYPE APP_TYPE##_instance;            \
    extern "C" ::enableit::BoardApp&                \
    get_##APP_TYPE##_app() {                        \
        return APP_TYPE##_instance;                 \
    }
