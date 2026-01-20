#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <functional>

class OtaHttpServer {
public:
    using UploadCallback = std::function<void(size_t bytesReceived, size_t totalSize)>;
    using FinishCallback = std::function<void(bool success, const String& message)>;
    using NotifyCallback = std::function<void(const String& message)>;

    explicit OtaHttpServer(uint16_t port);

    // Start the HTTP server on the given port with a session token
    void begin(const String& token);

    // Stop the HTTP server
    void end();

    // Set OTA upload progress and finish callbacks
    void onUploadProgress(UploadCallback cb);
    void onUploadFinished(FinishCallback cb);
    void onNotify(NotifyCallback cb);

    // Returns true if the server is running
    bool isRunning() const;

    // Returns the OTA URL (if available)
    String url() const;

private:
    void installRoutes();
    void handleRoot();
    void handleUpdate();
    void handleUpdateUpload();
    void handleUpdateRaw();

    // CORS handling
    void handleCors();
    void sendCorsHeaders();

    WebServer _server;
    uint16_t _port = 0;
    String _token;
    String _url;
    bool _running = false;

    UploadCallback _uploadCb = nullptr;
    FinishCallback _finishCb = nullptr;
    NotifyCallback _notifyCb = nullptr;

    // Internal state for upload
    bool     _uploading = false;
    size_t   _bytesReceived = 0;
    size_t   _expectedSize = 0;
};