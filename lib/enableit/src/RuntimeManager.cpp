#include "RuntimeManager.h"
#include <WiFi.h>
#include <WifiHal.h>
#include <ESPmDNS.h>
#if defined(INSIGHTS_SUPPORT)
#include <Insights.h>
#endif

#define WIFI_CHECK_DELAY            500 // 500ms delay
#define MAX_WIFI_CONNECT_ATTEMPTS   20

#if THINGSBOARD_SUPPORT
#include <ThingsBoard.h>
#endif

namespace enableit {

RuntimeManager::RuntimeManager(Board& board)
    : board_(board)
#if THINGSBOARD_SUPPORT
    , tb_(wifiClient_, MAX_MESSAGE_SIZE)
#endif
{}

bool RuntimeManager::enableWifi(const BootConfig& config) {
    if (wifiOn_) {
        log_w("WiFi already active");
        return true;
    }

    log_d("Activating WIFI");
    bool started = false;
    if (config.apMode) {
        log_i("Activating WiFi AP SSID[%s]", config.wifiSsid.c_str());
        WifiConfig cfg{config.wifiSsid.c_str(), config.wifiPassword.c_str()};
        started = board_.wifi().startAp(cfg);
    } else {
        log_i("Wifi init, connecting to[%s]", config.wifiSsid.c_str());
        WifiConfig cfg{config.wifiSsid.c_str(), config.wifiPassword.c_str()};
        started = board_.wifi().startSta(cfg);

        int count = 0;
        while ((WiFi.status() != WL_CONNECTED) && (count < MAX_WIFI_CONNECT_ATTEMPTS)) {
            delay(WIFI_CHECK_DELAY);
            log_i(".");
            count++;
        }
        log_i("");
        if (WiFi.status() == WL_CONNECTED) {
            log_i("WiFi connected");
            log_i("IP address: [%s]", WiFi.localIP().toString().c_str());
        } else {
            log_e("Wifi init failed");
            log_i("Scanning for known wifi");
            int n = WiFi.scanNetworks();
            log_i("Found %d networks", n);
            for (int i = 0; i < n; i++) {
                log_i("SSID[%s] RSSI[%d], Encryption[%s]", WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            }
            wifiOn_ = false;
            return false;
        }
    }

    if (!started) {
        log_e("WiFi HAL failed to start");
        wifiOn_ = false;
        return false;
    }

    // mDNS
    if (!mdnsOn_) {
        if (!MDNS.begin(config.mdnsHostname.c_str())) {
            log_e("Error setting up MDNS responder!");
        } else {
            log_i("mDNS responder started");
            mdnsOn_ = true;
        }
    }

#if defined(INSIGHTS_SUPPORT)
    // Insights
    if (config.insights && !insightsOn_) {
        if (Insights.begin(config.insightsKey.c_str())) {
            log_i("=========================================");
            log_i("ESP Insights enabled Node ID %s", Insights.nodeID());
            log_i("=========================================");
            insightsOn_ = true;
        } else {
            log_i("=========================================");
            log_i("ESP Insights enable failed");
            log_i("=========================================");
        }
    } else if (!config.insights) {
        log_w("ESP Insights disabled, requires WiFi active to enable it");
    }
#endif

    wifiOn_ = true;
    return true;
}

void RuntimeManager::disableWifi(const BootConfig& config) {
    if (!wifiOn_) {
        log_w("WiFi already deactivated");
        return;
    }

    log_i("Deactivating WIFI");

#if defined(INSIGHTS_SUPPORT)
    if (insightsOn_) {
        log_i("Stopping insights");
        Insights.end();
        insightsOn_ = false;
    }
#endif

    // No explicit MDNS stop in ESP-IDF/Arduino, but reset flag
    mdnsOn_ = false;

    board_.wifi().stop();

    log_i("Wifi disabled");
    wifiOn_ = false;
}

bool RuntimeManager::enableBle() {
    if (bleOn_) return true;
    // BtServer is started in constructor; if you need to restart, add log_iic here
    btServer_.init();
    bleOn_ = true;
    return true;
}

void RuntimeManager::disableBle() {
    if (!bleOn_) return;
    btServer_.end();
    // If BtServer supports stopping, call it here
    bleOn_ = false;
}

void RuntimeManager::startNormalMode(const BootConfig& config) {
    enableBle();
    enableWifi(config);
}

void RuntimeManager::startProvisioningMode(const BootConfig& config) {
    enableBle();
    // For provisioning, you may want AP or STA off; here we disable WiFi
    disableWifi(config);
}

void RuntimeManager::stopAll(const BootConfig& config) {
    disableBle();
    disableWifi(config);
}

#if defined(THINGSBOARD_SUPPORT)
bool RuntimeManager::enableThingsBoard(const BootConfig& config) {
    if (thingsBoardOn_) return true;
    if (!wifiOn_) {
        log_w("ThingsBoard: WiFi not active, cannot connect");
        return false;
    }
    if (config.deviceid.isEmpty() || config.thingsboard.isEmpty() || config.devicetoken.isEmpty()) {
        log_w("ThingsBoard: Missing configuration");
        return false;
    }
    if (!tb_.connected()) {
        log_d("Connecting to ThingsBoard[%s] with token [%s]", config.thingsboard.c_str(), config.devicetoken.c_str());
        if (!tb_.connect(config.thingsboard.c_str(), config.devicetoken.c_str(), THINGSBOARD_PORT, config.deviceid.c_str())) {
            log_e("Failed to connect to ThingsBoard");
            thingsBoardOn_ = false;
            return false;
        }
        tb_.sendAttributeString("macAddress", WiFi.macAddress().c_str());
        tb_.sendAttributeInt("rssi", WiFi.RSSI());
        tb_.sendAttributeString("bssid", WiFi.BSSIDstr().c_str());
        tb_.sendAttributeString("localIp", WiFi.localIP().toString().c_str());
        tb_.sendAttributeString("ssid", WiFi.SSID().c_str());
        tb_.sendAttributeInt("channel", WiFi.channel());
    }
    thingsBoardOn_ = tb_.connected();
    return thingsBoardOn_;
}

void RuntimeManager::disableThingsBoard() {
    if (thingsBoardOn_) {
        tb_.disconnect();
        thingsBoardOn_ = false;
    }
}

bool RuntimeManager::thingsBoardConnected() const {
    return const_cast<decltype(tb_)&>(tb_).connected();
}
#endif

// Setup RuntimeManager
RuntimeManager runtime(board);

} // namespace enableit
