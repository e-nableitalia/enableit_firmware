#include "OtaHttpServer.h"
#include <Update.h>
#include <RuntimeManager.h>

OtaHttpServer::OtaHttpServer(uint16_t port)
    : _server(port), _port(port)
{}

void OtaHttpServer::begin(const String& token) {
    if (_running) return;
    _token = token;

    BootConfig config;
    config.apMode = false;
    config.wifiSsid = "OTA_Upload";
    config.wifiPassword = ""; // Open network for simplicity
    enableit::runtime.enableWifi(config);

    if (!enableit::runtime.wifiOn()) {
        if (_finishCb) _finishCb(false, "WiFi not enabled");
        return;
    }

    installRoutes();
    _server.begin();
    _running = true;

    _url = "http://" + enableit::runtime.getIp().toString() + ":" + String(_port) + "/update?token=" + _token;
}

void OtaHttpServer::end() {
    if (!_running) return;
    _server.stop();
    _running = false;

    enableit::runtime.disableWifi();
}

void OtaHttpServer::handleClient() {
    if (_running) _server.handleClient();
}

void OtaHttpServer::onUploadProgress(UploadCallback cb) {
    _uploadCb = cb;
}

void OtaHttpServer::onUploadFinished(FinishCallback cb) {
    _finishCb = cb;
}

void OtaHttpServer::onNotify(NotifyCallback cb) {
    _notifyCb = cb;
}

bool OtaHttpServer::isRunning() const {
    return _running;
}

String OtaHttpServer::url() const {
    return _url;
}

void OtaHttpServer::installRoutes() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/update", HTTP_GET, [this]() { handleUpdate(); });
    _server.on("/update", HTTP_POST, [this]() { handleUpdate(); },
        [this]() { handleUpdateUpload(); });
    _server.on("/update/raw", HTTP_POST, [this]() { handleUpdateRaw(); });

    // Add OPTIONS handler for /update to support CORS preflight (e.g., Chrome web page OTA)
    _server.on("/update", HTTP_OPTIONS,
        [this]() {
            log_i("ORIGIN1 %s", _server.header("Origin").c_str());
            _server.sendHeader("Access-Control-Allow-Origin", _server.header("Origin"), true);
            _server.sendHeader("Access-Control-Allow-Headers", "X-OTA-Token", false);
            _server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS", false);
            _server.send(200, "text/plain", "Yo1");
        },
        [this]() {
            log_i("ORIGIN2 %s", _server.header("Origin").c_str());
            _server.sendHeader("Access-Control-Allow-Origin", _server.header("Origin"), true);
            _server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS", false);
            _server.send(200, "text/plain", "Yo2");
        }
    );

    _server.onNotFound([this]() { handleCors(); _server.send(404, "text/plain", "Not found"); });
}

void OtaHttpServer::handleRoot() {
    sendCorsHeaders();
    _server.send(200, "text/html", "<h1>OTA Update Server</h1>");
}

void OtaHttpServer::handleUpdate() {
    sendCorsHeaders();
    _server.send(200, "text/html",
        "<form method='POST' action='/update' enctype='multipart/form-data'>"
        "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
        "</form>");
}

void OtaHttpServer::handleUpdateUpload() {
    HTTPUpload& upload = _server.upload();
    sendCorsHeaders();
    if (upload.status == UPLOAD_FILE_START) {
        _uploading = true;
        _bytesReceived = 0;
        _expectedSize = upload.totalSize;
        if (_notifyCb) {
            String msg = "[OTA] upload start";
            if (_expectedSize)
                msg += " total=" + String(_expectedSize);
            _notifyCb(msg);
        }
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            if (_finishCb) _finishCb(false, "[OTA] Update.begin failed");
            return;
        }
        // Progress iniziale
        if (_notifyCb) {
            String msg;
            if (_expectedSize > 0) {
                msg = "OTA_PROGRESS 0 0/" + String(_expectedSize);
            } else {
                msg = "OTA_PROGRESS_BYTES 0";
            }
            _notifyCb(msg);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
            if (_finishCb) _finishCb(false, "[OTA] Update.write failed");
            return;
        }
        _bytesReceived += upload.currentSize;
        // Progress update (throttled)
        if (_notifyCb) {
            String msg;
            if (_expectedSize > 0) {
                uint8_t pct = (uint8_t)((_bytesReceived * 100ULL) / _expectedSize);
                if (pct > 100) pct = 100;
                msg = "OTA_PROGRESS " + String(pct) + " " +
                      String(_bytesReceived) + "/" + String(_expectedSize);
            } else {
                msg = "OTA_PROGRESS_BYTES " + String(_bytesReceived);
            }
            _notifyCb(msg);
        }
        if (_uploadCb) _uploadCb(_bytesReceived, _expectedSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        // Final progress update
        if (_notifyCb) {
            String msg;
            if (_expectedSize > 0) {
                msg = "OTA_PROGRESS 100 " +
                      String(_bytesReceived) + "/" + String(_expectedSize);
            } else {
                msg = "OTA_PROGRESS_BYTES " + String(_bytesReceived);
            }
            _notifyCb(msg);
        }
        _uploading = false;
        if (Update.end(true)) {
            if (_notifyCb) _notifyCb("[OTA] upload complete, size=" + String(upload.totalSize));
            if (_finishCb) _finishCb(true, "OTA_DONE");
        } else {
            if (_notifyCb) _notifyCb(String("[OTA] Update.end failed: ") + Update.errorString());
            if (_finishCb) _finishCb(false, "OTA_ERROR");
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end(false);
        if (_notifyCb) _notifyCb("[OTA] upload aborted");
        if (_finishCb) _finishCb(false, "OTA_ABORTED");
        _uploading = false;
    }
}

void OtaHttpServer::handleUpdateRaw() {
    sendCorsHeaders();
    _server.send(501, "text/plain", "Raw update not implemented");
}

void OtaHttpServer::handleCors() {
    sendCorsHeaders();
}

void OtaHttpServer::sendCorsHeaders() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
}