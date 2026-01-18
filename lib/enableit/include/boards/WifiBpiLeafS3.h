#pragma once

#ifdef ARDUINO_BPI_LEAF_S3

#include <Wifi.h>
#include <WiFiHal.h>

namespace enableit {

class WifiBpiLeafS3 : public Wifi {
public:
    bool startAp(const WifiConfig& cfg) override;
    bool startSta(const WifiConfig& cfg) override;
    void stop() override;
    String getIpAddress() override {
        return WiFi.localIP().toString();
    }
};

} // namespace enableit

#endif