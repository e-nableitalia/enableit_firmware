//
// BoardApp class definition (abstract board application)
//
// Author: A.Navatta / e-Nable Italia

#pragma once

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

class BoardApp {
public:
    virtual void enter() = 0;
    virtual void process() = 0;
    virtual void leave() = 0;
    virtual const char *name() = 0;
    void changeApp(const char *name);
    BoardApp **apps();
    bool hasApp(const char *state_name);
};

} // namespace enableit

#define BOARDAPP_INSTANCE(APP_TYPE)                 \
    static APP_TYPE APP_TYPE##_instance;            \
    extern "C" ::enableit::BoardApp&                \
    get_##APP_TYPE##_app() {                        \
        return APP_TYPE##_instance;                 \
    }
