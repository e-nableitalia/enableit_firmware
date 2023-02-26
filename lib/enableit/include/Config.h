//
// Config: Generic board config
//
// Author: A.Navatta / e-Nable Italia

#ifndef CONFIG_H

#define CONFIG_H

#include <Arduino.h>

#define CONFIG_VERSION  "1.0";

// parameters supported
// timeout      int, boot timeout in seconds, default: 3 seconds
// devmode      bool, development mode, enables OTA update and other dev features, default: false
// wifisid      string, wifi network id, default: enableit
// wifipwd      string, wifi network password, default: enableit
// apmode       bool, Access Point mode enabled, default: false
// otaurl       string, URL for OTA Update
// password     string, maintenance password

class Config
{
public:
    Config();
    void init();
    void save();
    void load(char *json);
    void set(const char *param, const char *value);
    String dump();
    void fullDump();
    void setSecretKey(String pwd);
    void unlock(String pwd);
    void enable(String passwd);
    void disable();
    bool isPrivileged();
    void reset();

    String wifiSsid;
    String wifiPassword;
    String otaurl;
    String password;
    String insightsKey;
    String secretKey;
    String devApp;
    String mainApp;
    unsigned int  bootTimeout;
    unsigned int baudRate;
    bool wifi;
    bool devMode;
    bool apMode;
    bool insights;
private:
    int getParam(const char *p);
    bool privileged;
    bool stored;
    int size;
 };

#if !defined(NO_GLOBAL_INSTANCES)
extern Config config;
#endif

#endif // CONFIG_H