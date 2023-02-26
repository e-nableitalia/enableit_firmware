#include <Arduino.h>
#include <debug.h>
#include <WiFi.h>
#include <HttpsOTAUpdate.h>
#include <OTAUpdateApp.h>
#include <Config.h>

static const char *server_certificate = "";

static HttpsOTAStatus_t otastatus;

static bool otainit = false;

void HttpEvent(HttpEvent_t *event)
{
    switch(event->event_id) {
        case HTTP_EVENT_ERROR:
            DBG("Http Event Error");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            DBG("Http Event On Connected");
            break;
        case HTTP_EVENT_HEADER_SENT:
            DBG("Http Event Header Sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            DBG("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            break;
        case HTTP_EVENT_ON_FINISH:
            DBG("Http Event On Finish");
            break;
        case HTTP_EVENT_DISCONNECTED:
            DBG("Http Event Disconnected");
            break;
    }
}

void OTAInit() {
    DBG("Starting OTA Update");
    HttpsOTA.onHttpEvent(HttpEvent);
    HttpsOTA.begin(config.otaurl.c_str(), nullptr, true); 
    otainit = true;
}

void OTAUpdateApp::enter() {
    DBG("Entering in OTA Update state");
    otainit = false;
}
void OTAUpdateApp::leave() {
    DBG("Leaving boot Loader state");
}

void OTAUpdateApp::process() {
    //DBG("Called process in BootLoader state");
    if (!otainit)
        OTAInit();
    else {
        otastatus = HttpsOTA.status();
        if(otastatus == HTTPS_OTA_SUCCESS) { 
            DBG("Firmware written successfully. Rebooting device");
            changeApp(STATE_REBOOT);
        } else if(otastatus == HTTPS_OTA_FAIL) { 
            DBG("Firmware Upgrade Fail");
            changeApp(APP_BOOT);
        }
    }
}

const char *OTAUpdateApp::name() {
    return APP_OTAUPDATE;
}
