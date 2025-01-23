#include <Arduino.h>
#include <Preferences.h>
#include <Esp.h>
#include <ArduinoJson.h>
#include <Cipher.h>
#include <Console.h>
#include <Config.h>
#include <base64.h>
#include <libb64/cdecode.h>

const char insights_auth_key[] = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VyIjoiMTJlZGZkYjMtZTc4OC00MjNiLTljZjEtYjdhODA0Mzk2OWM4IiwiZXhwIjoxOTg4NDk0MjkzLCJpYXQiOjE2NzMxMzQyOTMsImlzcyI6ImUzMjJiNTljLTYzY2MtNGU0MC04ZWEyLTRlNzc2NjU0NWNjYSIsInN1YiI6IjJlZDJiNjU4LTM2MjItNDU5Ni04NzJlLWM2N2RlMDg5Nzc1YyJ9.KY2Tk74vsEqWY18CsescSTIqDh9WPwWnCIdVFYfYiGxMJC8BfksXV3Yup8aFN-gYCHKX-rD4MyvwyEgA19fOt4kSRBH8x5BnYj19e97R3Lf_x7AQO9-zC-alSmMFb8dt6jWgSsXkGHX17udVDKiOC3JgFQgwqO5cVhHP4JFXUd3q-0OlRhHiW8ixnW9dTQhDhrtqA3lTljzAT8V51uoWnbhcXLb0hLfjXMIPMDbDT8BUx9VK5Sz4JUeSAi2QvfpUB_H77h2E_f9QbKRKRn0RapPf92rXB2vfQRPDxqdq-ii_LpiFWAfP9SlnbSVLzQaXuWrcYKtHv-mm2RSRE6cNKQ";

const char *url = "https://example.com/firmware.bin"; //state url of your firmware image

const char *secretkey = "7SgNsZ3^EX^sgK#n";

#define TYPE_STRING     0 
#define TYPE_INT        1
#define TYPE_BOOL       2
#define TYPE_ENCRYPTED  3

#define STORAGE       "enableit"
#define SIGNATURE     "configv1"
#define SIGNATURE_LEN   sizeof(SIGNATURE)

#if !defined(NO_GLOBAL_INSTANCES)
Config config;
#endif

typedef union  {
    unsigned int *intvalue;
    String *charvalue;
    bool *boolvalue;
} multiValue;

typedef union  {
    unsigned int intvalue;
    char *charvalue;
    bool boolvalue;
} multiDefaultValue;

String encode(String value) {
    Cipher * cipher = new Cipher();

    cipher->setKey((char *)config.secretKey.c_str());
    String encoded = base64::encode(cipher->encryptString(value));
    DBG("Encoding[%s], key[%s], encoded value[%s]",
        value.c_str(), config.secretKey.c_str(), encoded.c_str());

    return encoded;
}

String decode(String value) {
   char buffer[256];
    memset(buffer,0,256);
    Cipher * cipher = new Cipher();

    cipher->setKey((char *)config.secretKey.c_str());

    int decoded_len = base64_decode_chars(value.c_str(),value.length(),buffer);

    String decoded = cipher->decryptString(buffer);
    DBG("Encoded[%s], key[%s], decoded value[%s]",
        value.c_str(), config.secretKey.c_str(), decoded.c_str());

    return decoded;
}

class Param {
public:
    Param(const char *n, unsigned int &var, bool p, const int d) { name = n; privileged = p; type = TYPE_INT; value.intvalue = &var; defaultvalue.intvalue = (int)d; }
    Param(const char *n, bool &var, bool p, const bool d) { name = n; privileged = p; type = TYPE_BOOL; value.boolvalue = &var; defaultvalue.boolvalue = (bool)d; }
    Param(const char *n, String &var, bool p, bool pwd, const char *d) { name = n; privileged = p; if (pwd) type = TYPE_ENCRYPTED; else type = TYPE_STRING; value.charvalue = &var; defaultvalue.charvalue = (char *)d; }
    void set(const char *v) {
        if ((!config.isPrivileged()) && (privileged)) {
            DBG("Privileged parameter[%s] not set", name);
            return;
        }        
        if (type == TYPE_STRING) {
            *(value.charvalue) = (char *)v;
        } else if (type == TYPE_INT) {
            *(value.intvalue) = (unsigned int)atoi(v);
        } else if (type == TYPE_BOOL) {
            *(value.boolvalue) = (bool)!strcasecmp("true", v);
        } else if (type == TYPE_ENCRYPTED) {
            *(value.charvalue) = encode(v);
        }
    };

    void reset() {
        if (type == TYPE_STRING) {
            *(value.charvalue) = defaultvalue.charvalue;
        } else if (type == TYPE_INT) {
            *(value.intvalue) = defaultvalue.intvalue;
        } else if (type == TYPE_BOOL) {
            *(value.boolvalue) = defaultvalue.boolvalue;
        } else if (type == TYPE_ENCRYPTED) {
            *(value.charvalue) = encode(defaultvalue.charvalue);
        }
    }

    const char *get() { 
        if ((type == TYPE_STRING) || (type == TYPE_ENCRYPTED)) {
            if (value.charvalue)
                value.charvalue->c_str();
            else return defaultvalue.charvalue;
        } else if (type == TYPE_INT) {
            if (value.intvalue) {
                String v(*(value.intvalue));
                return v.c_str();
            }
            else {
                String v(defaultvalue.intvalue);
                return v.c_str();
            }
        } else if (type == TYPE_BOOL) {
            if (value.boolvalue)
                return (*(value.boolvalue) ? "true" : "false");
            else return ((defaultvalue.boolvalue) ? "true" : "false");
        }     
    }
    void load(Preferences &p) { 
        if ((type == TYPE_STRING) || (type == TYPE_ENCRYPTED)) {
            *value.charvalue = p.getString(name,(const char *)defaultvalue.charvalue);
        } else if (type == TYPE_INT) {
            *value.intvalue = p.getInt(name,(const int)defaultvalue.charvalue);
        } else if (type == TYPE_BOOL) {
            *value.boolvalue = p.getBool(name,(const bool)defaultvalue.charvalue);
        } 
    };

    void store(Preferences &p) {
        if ((!config.isPrivileged()) && (privileged)) {
            DBG("Privileged parameter[%s] not stored", name);
            return;
        }
        if ((type == TYPE_STRING) || (type == TYPE_ENCRYPTED)) {
            p.putString(name, *value.charvalue);
        } else if (type == TYPE_INT) {
            p.putInt(name,*value.intvalue);
        } else if (type == TYPE_BOOL) {
            p.putBool(name,*value.boolvalue);
        }
    }

    const char *name;
    int type;
    bool privileged;
    multiValue value;
    multiDefaultValue defaultvalue;
};

Param params[] = {
    { "wifissid", config.wifiSsid, true, false, "enable-test" }, 
    { "wifipwd", config.wifiPassword, true, false, "enableit" }, 
    { "wifi", config.wifi, false, true }, 
    { "apmode", config.apMode, true, false },
    { "thingsboard", config.thingsboard, true, false, "dev.e-nableitalia.it" },
    { "deviceid", config.deviceid, true, false, "ed629b30-16dd-11ee-be0f-a557af31b048" },
    { "devicetoken", config.devicetoken, true, false, "bxj5xo14c80nxoxxsndu" },
    { "otaurl", config.otaurl, true, false, url },
    { "httpusername", config.httpUsername, true, false, "admin" },
    { "httppassword", config.httpPassword, true, false, "admin" },
    { "timeout", config.bootTimeout, false,  3 },
    { "app", config.mainApp, true, false, "kinetichand" },
    { "telnet", config.telnet, true,  true },
    { "devmode", config.devMode, true,  false },
    { "devapp", config.devApp, true, false, "otaweb" },
    { "password", config.password, true, true, "tDCKZkmnGseokNvfW/kOvQ==" },
    { "insights", config.insights, true,  false },
    { "insightskey", config.insightsKey, true, false, insights_auth_key },
    { "baudrate", config.baudRate, false, 115200 }
};

#define SIZE    (sizeof(params) / sizeof(Param))

Config::Config() {
}

bool Config::isPrivileged() {
    return privileged;
}

void Config::init() {
    DBG("Preferences init");

    // read default preferences
    Preferences preferences;

    preferences.begin(STORAGE, true);
    char buffer[SIGNATURE_LEN + 1];
    memset(buffer,0,SIGNATURE_LEN + 1);
    preferences.getBytes("signature", buffer, SIGNATURE_LEN);

    String key;

    if (!strcmp(buffer,SIGNATURE)) {
        LOG("Valid config found, loading...");
        for (int i = 0; i < SIZE; i++)
            params[i].load(preferences);
        key = preferences.getString("secretKey", secretkey);
        LOG("Config loaded");
    } else {
        LOG("Invalid config found, loading default");
        LOG("enable password is: 'secret', please change it before saving settings");
        for (int i = 0; i < SIZE; i++)
            params[i].reset();
        
        key = secretkey;
    }

    privileged = true;
    setSecretKey(key);
    privileged = false;

    preferences.end();

    DBG("Dumping config:");
    fullDump();
}

void Config::reset() {
    if (!privileged)
        return;
    DBG("Clearing preferences");
    Preferences preferences;
    preferences.begin(STORAGE, false);
    preferences.clear();
    preferences.end();
}

void Config::save() {
    DBG("Preferences saved");
    // read default preferences
    Preferences preferences;
    preferences.begin(STORAGE, false);
    for (int i = 0; i < SIZE; i++)
        params[i].store(preferences);
    
    preferences.putString("secretKey", secretKey);
    preferences.putBytes("signature", SIGNATURE, SIGNATURE_LEN);

    preferences.end();
}

void Config::setSecretKey(String pwd) {
    if (!privileged)
        return;
    pwd += secretkey;
    // normalize to 16 chars
    secretKey = pwd.substring(0,16);
}

void Config::unlock(String pwd) {
    String oldkey = secretKey;
    secretKey = secretkey;

    String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
    chipId.toUpperCase();

    String token = encode(secretkey + chipId);

    DBG("Token[%s]", token.c_str());
    if (!token.compareTo(pwd)) {
        DBG("Token validated, privileged mode on");
        privileged = true;
    }
    
    secretKey = oldkey;
}

void Config::load(char *json) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, json);

    for (int i = 0; i < SIZE; i++) {
        if (doc.containsKey(params[i].name)) {
            // set value        
            if ((!config.isPrivileged()) && (params[i].privileged)) {
                DBG("Privileged parameter[%s] not stored", params[i].name);
            } else
                params[i].set(doc[params[i].name].as<const char *>());
        }
    }
}

void Config::fullDump() {

    for (int i = 0; i < SIZE; i++) {
        if (params[i].type == TYPE_ENCRYPTED) {
            DBG("param[%s], encrypted value[%s]",params[i].name, (*(params[i].value.charvalue)).c_str());
        } else if (params[i].type == TYPE_STRING) {
            DBG("param[%s], string value[%s]",params[i].name, (*(params[i].value.charvalue)).c_str());
        } else if (params[i].type == TYPE_INT) {
            DBG("param[%s], int value[%d]",params[i].name, *(params[i].value.intvalue));
        } else if (params[i].type == TYPE_BOOL) {
            DBG("param[%s], bool value[%d]",params[i].name, *(params[i].value.boolvalue));
        }
    }

    DBG("Encryption password[%s]",config.secretKey.c_str());
    DBG("Privileged mode[%s]", privileged ? "enabled" : "disabled");
}

String Config::dump() {
    StaticJsonDocument<1024> doc;

    for (int i = 0; i < SIZE; i++) {
        if ((!config.isPrivileged()) && (params[i].privileged)) {
            DBG("Privileged parameter[%s] not stored", params[i].name);
        } else {
            if (params[i].type == TYPE_ENCRYPTED) {
                DBG("Adding encrypted param[%s], value[%s]",params[i].name, (*(params[i].value.charvalue)).c_str());
                doc[params[i].name] = *(params[i].value.charvalue);
            } else if (params[i].type == TYPE_STRING) {
                DBG("Adding string param[%s], value[%s]",params[i].name, (*(params[i].value.charvalue)).c_str());
                doc[params[i].name] = *(params[i].value.charvalue);
            } else if (params[i].type == TYPE_INT) {
                DBG("Adding int param[%s], value[%d]",params[i].name, *(params[i].value.intvalue));
                doc[params[i].name] = *(params[i].value.intvalue);
            } else if (params[i].type == TYPE_BOOL) {
                DBG("Adding bool param[%s], value[%d]",params[i].name, *(params[i].value.boolvalue));
                doc[params[i].name] = *(params[i].value.boolvalue);
            }
        }
    }

    String s;
    serializeJsonPretty(doc,s);
    DBG("Raw json: %s", s.c_str());
    return s;
}

int Config::getParam(const char *p) {
    for (int i = 0; i < SIZE; i++) {
        if (!strcasecmp(p, params[i].name))
            return i;
    }
    return -1;
}

void Config::set(const char *p, const char *v) {
    int index = getParam(p);

    if (index >= 0) {
        DBG("Setting param[%s], value[%s]", p, v);
        params[index].set(v);
    }
}

void Config::enable(String pass) {
    String cipherString = decode(password);

    if (!cipherString.compareTo(pass)) {
        DBG("Going in privileged user mode");
        privileged = true;
    } else {
        DBG("Failed to go in privileged user mode");
        privileged = false;
    }
}

void Config::disable() {
    DBG("Disabling privileged user mode");
    privileged = !privileged; //false;
}