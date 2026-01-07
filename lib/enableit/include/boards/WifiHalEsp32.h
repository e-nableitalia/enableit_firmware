#pragma once

#include <WifiHal.h>
#include <WiFi.h>

class WifiHalEsp32 : public WifiHal {
public:
    bool startAp(const WifiConfig& cfg) override;
    bool startSta(const WifiConfig& cfg) override;
    void stop() override;
    String getIpAddress() override {
        return WiFi.localIP().toString();
    }
};
