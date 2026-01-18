#include <Arduino.h>
#include <Console.h>
#include <WiFi.h>
#include <HttpsOTAUpdate.h>
#include "OTAUpdateApp.h"
#include <Config.h>

static const char *server_certificate = "";

static HttpsOTAStatus_t otastatus;

static bool otainit = false;

BOARDAPP_INSTANCE(OTAUpdateApp);

void HttpEvent(HttpEvent_t *event)
{
    switch(event->event_id) {
        case HTTP_EVENT_ERROR:
            log_d("Http Event Error");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            log_d("Http Event On Connected");
            break;
        case HTTP_EVENT_HEADER_SENT:
            log_d("Http Event Header Sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            log_d("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            break;
        case HTTP_EVENT_ON_FINISH:
            log_d("Http Event On Finish");
            break;
        case HTTP_EVENT_DISCONNECTED:
            log_d("Http Event Disconnected");
            break;
    }
}

void OTAInit() {
    log_d("Starting OTA Update");
    HttpsOTA.onHttpEvent(HttpEvent);
    HttpsOTA.begin(config.otaurl.c_str(), nullptr, true); 
    otainit = true;
}

void OTAUpdateApp::enter() {
    log_d("Entering in OTA Update state");
    otainit = false;
}
void OTAUpdateApp::leave() {
    log_d("Leaving boot Loader state");
}

void OTAUpdateApp::process() {
    //log_d("Called process in BootLoader state");
    if (!otainit)
        OTAInit();
    else {
        otastatus = HttpsOTA.status();
        if(otastatus == HTTPS_OTA_SUCCESS) { 
            log_d("Firmware written successfully. Rebooting device");
            changeApp(STATE_REBOOT);
        } else if(otastatus == HTTPS_OTA_FAIL) { 
            log_d("Firmware Upgrade Fail");
            changeApp(APP_BOOT);
        }
    }
}

const char *OTAUpdateApp::name() {
    return APP_OTAUPDATE;
}
