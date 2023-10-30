//
// BoardApp class definition (abstract board application)
//
// Author: A.Navatta / e-Nable Italia

#ifndef BOARD_APP_H

#define BOARD_APP_H

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

class BoardApp {
public:
    virtual void enter() = 0;
    virtual void process() = 0;
    virtual void leave() = 0;
    virtual const char *name() = 0;
    void changeApp(const char *name);
    BoardApp **apps();
};

#endif // BOARD_APP_H