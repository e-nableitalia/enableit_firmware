#pragma once


#include "Board.h"
#include "Wifi.h"
#include <memory>
#include "FeatureRegistry.h"
#include "ProtocolProcessor.h"
#include "BleCommandDispatcher.h"

#if defined(THINGSBOARD_SUPPORT)
#include <WiFiClient.h>
#include <ThingsBoard.h>

// MQTT port used to communicate with the server, 1883 is the default unencrypted MQTT port.
constexpr uint16_t THINGSBOARD_PORT = 1883U;

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint32_t MAX_MESSAGE_SIZE = 256U;
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
    String bleDeviceName = "eNableIt board";
    String bleServiceUuid = "";
    // Add other fields as needed
};

// BLEConfig: configuration for BLE characteristics
struct BLEConfig {
    uint32_t properties = 0; // Bitmask of properties (read, write, notify, etc.)
    String uuid;              // UUID for characteristic
};

namespace enableit {

class RuntimeManager {
public:
    RuntimeManager(Board& board);

    // Wi-Fi control (policy)
    bool enableWifi(const BootConfig& config);
    void disableWifi();

    // Combined modes (helper di policy)
    void startNormalMode(const BootConfig& config);
    void startProvisioningMode(const BootConfig& config);
    void stopAll();

    // BLE control (policy) with service UUID
    bool enableBle(String name, String uuid);
    void startBle();
    void disableBle();

    // BLE based protocols two-phase init
    void initProtocols(int count, const BLEConfig config[]);
    // BLE Command dispatcher registration, wrapped just to keep focused 
    // on the need to call those functions after enableBle and before startBle
    void registerBleCommandDispatcher(BleCommandDispatcher *c);
    
    void registerFeature(FeatureBase* feature);
    void unregisterFeature(const char* name);
    FeatureBase* getFeature(const char* name);

    // State query
    bool wifiOn() const { return wifiOn_; }
    bool bleOn() const { return bleOn_; }
    bool bleConnected() const { return BtServer::instance().isConnected(); }
    bool wifiConnected() const { return WiFi.status() == WL_CONNECTED; }

    IPAddress getIp() const { return ipAddress_; }

#if defined(THINGSBOARD_SUPPORT)
    bool enableThingsBoard(const BootConfig& config);
    void disableThingsBoard();
    bool thingsBoardConnected() const;
#endif

private:
    Board& board_;

    FeatureRegistry featureRegistry_;
    std::unique_ptr<ProtocolProcessor> protocol_;

    bool wifiOn_ = false;
    bool bleOn_ = false;
    bool mdnsOn_ = false;
    bool insightsOn_ = false;

    IPAddress ipAddress_;

#if defined(THINGSBOARD_SUPPORT)
    WiFiClient wifiClient_;
    ThingsBoard tb_;
    bool thingsBoardOn_ = false;
#endif
};

// BLEProtocolHandler skeleton
class BLEProtocolHandler : public BleCharacteristicHandler {
public:
    explicit BLEProtocolHandler(ProtocolProcessor& processor);
    void onWrite(BLECharacteristic* ch) override;
    void onRead(BLECharacteristic* ch) override;
    void onSubscribe() override {
        log_i("BLEProtocolHandler: Client subscribed to characteristic");
    }

    void onUnsubscribe() override {
        log_i("BLEProtocolHandler: Client unsubscribed from characteristic");
    }
private:
    ProtocolProcessor& processor_;
};

extern RuntimeManager runtime;

} // namespace enableit
