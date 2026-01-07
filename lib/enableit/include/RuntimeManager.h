#pragma once

#include "Board.h"
#include "BtServer.h"
#include "WifiHal.h"

#define THINGSBOARD_SUPPORT 1
// MQTT port used to communicate with the server, 1883 is the default unencrypted MQTT port.
constexpr uint16_t THINGSBOARD_PORT = 1883U;

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint32_t MAX_MESSAGE_SIZE = 256U;

#if THINGSBOARD_SUPPORT
#include <WiFiClient.h>
#include <ThingsBoard.h>
#endif

// BootConfig: minimal config needed for runtime policy
struct BootConfig {
    String wifiSsid;
    String wifiPassword;
    bool apMode = false;
    bool insights = false;
    String insightsKey;
    String deviceid;
    String thingsboard;
    String devicetoken;
    String mdnsHostname = "esp32";
    // Add other fields as needed
};

class RuntimeManager {
public:
    RuntimeManager(Board& board, BtServer& btServer);

    // Wi-Fi control (policy)
    bool enableWifi(const BootConfig& config);
    void disableWifi(const BootConfig& config);

    // BLE control (policy)
    bool enableBle();
    void disableBle();

    // Combined modes (helper di policy)
    void startNormalMode(const BootConfig& config);
    void startProvisioningMode(const BootConfig& config);
    void stopAll(const BootConfig& config);

    // State query
    bool wifiOn() const { return wifiOn_; }
    bool bleOn() const { return bleOn_; }

#if THINGSBOARD_SUPPORT
    bool enableThingsBoard(const BootConfig& config);
    void disableThingsBoard();
    bool thingsBoardConnected() const;
#endif

private:
    Board& board_;
    BtServer& btServer_;
    bool wifiOn_ = false;
    bool bleOn_ = false;
    bool mdnsOn_ = false;
    bool insightsOn_ = false;

#if THINGSBOARD_SUPPORT
    WiFiClient wifiClient_;
    ThingsBoard tb_;
    bool thingsBoardOn_ = false;
#endif
};

extern RuntimeManager runtime;
