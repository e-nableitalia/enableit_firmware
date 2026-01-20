#pragma once

#include <Arduino.h>
#include <functional>
#include <enableit.h>
#include <BoardApp.h>
#include "OtaHttpServer.h"

static constexpr uint16_t OTA_PORT = 8080;

// Minimal OTA App as a BoardApp
class OtaApp : public enableit::BoardApp {
public:
    explicit OtaApp(uint16_t port = OTA_PORT);

    void enter() override;
    void process() override { /* nothing to do here */ }; 
    void leave() override;
    const char* name() override { return APP_OTAUPDATE; }

    String getInfo(String key) const override;
    void setInfo(String key, String value) override;

    bool hasCapability(const char* cap) const override {
        if (strcmp(cap, enableit::capabilities::Notify) == 0) {
            return true;
        }
        return enableit::BoardApp::hasCapability(cap);
    }

    void setNotifyFn(NotifyFn fn) override {
        notifyFn_ = fn;
    }
private:
    enum class State : uint8_t { Idle, ConnectingWifi, Ready, Error };

    void setState(State s, const String& msg = "");
    void sendReply(const String& s);
    void maybeSendProgress(bool force = false);
    void taskMain();
    bool ensureWifi(const String& ssid, const String& pass);
    static void OtaAppTaskTrampoline(void* param);



    // Progress / timeouts
    bool     _otaSuccess = false;
    bool     _uploading = false;
    size_t   _bytesReceived = 0;
    size_t   _expectedSize = 0;
    uint32_t _lastProgressMs = 0;
    uint32_t _lastActivityMs = 0;
    uint32_t _sessionStartMs = 0;
    uint8_t  _lastPctSent = 255;

    State _state = State::Idle;
    String _stateMsg;
    String _token;
    String _url;
    String _ssid;
    String _pass;

    TaskHandle_t _task = nullptr;
    volatile bool _stopTask = false;

    // Nuova proprietà per fallback
    String _previousAppName;

    static constexpr uint32_t OTA_IDLE_TIMEOUT_MS    = 10 * 1000;
    static constexpr uint32_t OTA_SESSION_TIMEOUT_MS = 2  * 60*1000;
    static constexpr uint32_t OTA_PROGRESS_MIN_MS    = 300;
    static constexpr uint8_t  OTA_PROGRESS_STEP_PCT  = 2;

    OtaHttpServer _server;
    NotifyFn      notifyFn_ = nullptr;
};