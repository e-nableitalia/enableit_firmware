#include <RuntimeManager.h>
#include "OtaApp.h"
#include "OtaHttpServer.h"
#include <esp_system.h>
#include <esp_log.h>

BOARDAPP_INSTANCE(OtaApp);

static const char* TAG = "OtaApp";

OtaApp::OtaApp(uint16_t port)
    :  _server(port)
{
    log_d("[OtaApp] Constructor called");
}

void OtaApp::OtaAppTaskTrampoline(void* arg) {
    log_d("[OtaApp] OtaAppTaskTrampoline start");
    static_cast<OtaApp*>(arg)->taskMain();
    log_d("[OtaApp] OtaAppTaskTrampoline end");
    vTaskDelete(nullptr);
}

void OtaApp::enter() {
    log_i("[OtaApp] enter() called");
    _token = "session-token"; // TODO: generate real token if needed
    // Generate a random token (16 hex chars)
    uint32_t r1 = (uint32_t)esp_random();
    uint32_t r2 = (uint32_t)esp_random();
    char tokenBuf[17];
    snprintf(tokenBuf, sizeof(tokenBuf), "%08lx%08lx", (unsigned long)r1, (unsigned long)r2);
    _token = String(tokenBuf);
    _token = String(tokenBuf);
    _url.clear();
    _otaSuccess = false;
    _uploading = false;
    _bytesReceived = 0;
    _expectedSize = 0;
    _lastProgressMs = 0;
    _lastActivityMs = millis();
    _lastPctSent = 255;
    _sessionStartMs = millis();
    
    //setState(State::ConnectingWifi, "scheduled");

    // Avvia task dedicato per setup WiFi e server
    if (_task) {
        log_d("[OtaApp] Task already running, skipping enter");
        // già in esecuzione
        return;
    }
    _stopTask = false;
    BaseType_t ok = xTaskCreatePinnedToCore(
        OtaApp::OtaAppTaskTrampoline,
        "ota_app_task",
        6144,
        this,
        1,
        &_task,
        1);
    if (ok != pdPASS) {
        log_e("[OtaApp] Failed to create OTA task");
        _task = nullptr;
        //setState(State::Error, "failed to create task");
    } else {
        log_i("[OtaApp] OTA task created successfully");
    }
}

void OtaApp::taskMain() {
    log_i("[OtaApp] taskMain() started");
    // Setup WiFi (blocking, safe qui)
    // ---- WiFi + server start happens HERE (safe task context) ----
    if (!ensureWifi(_ssid, _pass)) {
        log_e("[OtaApp] ensureWifi failed");
        _task = nullptr;
        return;
    }

    // Setup server e callback (come in setNotifyFn)
    _server.onUploadProgress([this](size_t bytesReceived, size_t totalSize) {
        log_d("[OtaApp] onUploadProgress: %u/%u", (unsigned)bytesReceived, (unsigned)totalSize);
        _uploading = true;
        _bytesReceived = bytesReceived;
        _expectedSize = totalSize;
        _lastActivityMs = millis();
        maybeSendProgress(false);
    });

    _server.onUploadFinished([this](bool success, const String& msg) {
        log_i("[OtaApp] onUploadFinished: success=%d, msg=%s", success, msg.c_str());
        _uploading = false;
        _lastActivityMs = millis();
        if (success) {
            setState(State::Ready, "OTA update successful");
            sendReply(msg);
            _otaSuccess = true;
        } else {
            setState(State::Error, msg);
            sendReply(msg);
            _server.end();
        }
    });

    _server.onNotify([this](const String& msg) {
        log_d("[OtaApp] onNotify: %s", msg.c_str());
        sendReply(msg);
    });

    _server.begin(_token);
    _url = _server.url();
    setState(State::Ready, _url);
    sendReply("OTA_READY " + _url);

    // Loop di servizio
    _lastActivityMs = millis();
    _sessionStartMs = _lastActivityMs;
    log_i("[OtaApp] Service loop started");
    while (!_stopTask) {
        vTaskDelay(pdMS_TO_TICKS(5));
        uint32_t now = millis();

        if (_otaSuccess) {
            log_i("[OtaApp] OTA success, rebooting...");
            sendReply("[OTA] Rebooting...");
            delay(500);
            esp_restart();
        }
        if (_uploading) {
            maybeSendProgress(false);
        }
        if (!_uploading && (now - _lastActivityMs > OTA_IDLE_TIMEOUT_MS)) {
            log_w("[OtaApp] Idle timeout");
            sendReply("[OTA] Idle timeout");
            break;
        }
        if (now - _sessionStartMs > OTA_SESSION_TIMEOUT_MS) {
            log_w("[OtaApp] Session timeout");
            sendReply("[OTA] Session timeout");
            break;
        }
    }
    log_i("[OtaApp] Service loop ended, shutting down server");
    _server.end();
    setState(State::Idle, "stopped");
    _task = nullptr;
}

bool OtaApp::ensureWifi(const String& ssid, const String& pass) {
    log_i("[OtaApp] ensureWifi(ssid='%s')", ssid.c_str());
    // Logica simile a OtaWifiUploadServer::ensureWifi
    if (enableit::runtime.wifiOn()) {
        log_i("[OtaApp] WiFi already connected");
        sendReply("[OTA] WiFi already connected, IP=" + WiFi.localIP().toString());
        return true;
    }
    if (ssid.isEmpty()) {
        log_e("[OtaApp] No SSID provided");
        setState(State::Error, "WiFi not connected and no SSID provided");
        return false;
    }

    setState(State::ConnectingWifi, "connecting to WiFi SSID='" + ssid + "'");
    BootConfig config;
    config.wifiSsid = ssid;
    config.wifiPassword = pass;
    config.apMode = false;
    enableit::runtime.enableWifi(config);
    
    if (!enableit::runtime.wifiOn()) {
        log_e("[OtaApp] WiFi connect timeout");
        setState(State::Error, "WiFi connect timeout");
        return false;
    }
    log_i("[OtaApp] WiFi connected, IP=%s", WiFi.localIP().toString().c_str());
    sendReply("[OTA] WiFi connected, IP=" + WiFi.localIP().toString());
    return true;
}

void OtaApp::leave() {
    log_i("[OtaApp] leave() called");
    _stopTask = true;
    // attendi che il task termini e resetta stato
    for (int i = 0; i < 50 && _task; ++i) {
        delay(10);
    }
    _server.end();
    _uploading = false;
    _bytesReceived = 0;
    _expectedSize = 0;
    _lastProgressMs = 0;
    _lastActivityMs = 0;
    _lastPctSent = 255;
    _otaSuccess = false;
    _sessionStartMs = 0;
    setState(State::Idle, "");
    sendReply("[OTA] Server stopped");
}

String OtaApp::getInfo(String key) const {
    log_d("[OtaApp] getInfo(%s)", key.c_str());
    if (key == "url") {
        return _url;
    } else if (key == "state") {
        return String(static_cast<int>(_state));
    } else if (key == "state_msg") {
        return _stateMsg;
    } else if (key == "ssid") {
        return _ssid;
    } else if (key == "pass") {
        return _pass;
    }
    return enableit::BoardApp::getInfo(key);
}

void OtaApp::setInfo(String key, String value) {
    log_d("[OtaApp] setInfo(%s, %s)", key.c_str(), value.c_str());
    if (key == "ssid") {
        _ssid = value;
    } else if (key == "pass") {
        _pass = value;
    } else {
        enableit::BoardApp::setInfo(key, value);
    }
}

void OtaApp::setState(State s, const String& msg) {
    //log_i("[OtaApp] setState(%d, %s)", (int)s, msg.c_str());
    _state = s;
    _stateMsg = msg;

    String st;
    switch (_state)
    {
    case State::Idle:
        st = "IDLE";
        break;
    case State::ConnectingWifi:
        st = "CONNECTING_WIFI";
        break;
    case State::Ready:
        st = "READY";
        break;
    case State::Error:
        st = "ERROR";
        break;
    }
    if (_stateMsg.length())
        st += " - " + _stateMsg;
    sendReply("[OTA] " + st);
}

void OtaApp::sendReply(const String& s) {
    log_d("[OtaApp] sendReply: %s", s.c_str());
    if (notifyFn_) 
        notifyFn_(s);
}

void OtaApp::maybeSendProgress(bool force) {
    log_d("[OtaApp] maybeSendProgress(force=%d): %u/%u", force, (unsigned)_bytesReceived, (unsigned)_expectedSize);
    if (!_uploading || _expectedSize == 0) return;
    uint32_t now = millis();
    uint8_t pct = (_bytesReceived * 100) / _expectedSize;
    if (!force) {
        if (_lastPctSent != 255 && pct < _lastPctSent + OTA_PROGRESS_STEP_PCT &&
            now - _lastProgressMs < OTA_PROGRESS_MIN_MS) {
            return;
        }
    }
    sendReply("OTA_PROGRESS " + String(pct) + " " + String(_bytesReceived) + "/" + String(_expectedSize));
    sendReply("OTA_PROGRESS_BYTES " + String(_bytesReceived));
    _lastPctSent = pct;
    _lastProgressMs = now;
}