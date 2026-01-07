#pragma once

#include <Arduino.h>

struct WifiConfig {
    const char* ssid;
    const char* password;
};

class WifiHal {
public:
    virtual ~WifiHal() = default;

    // Primitive operative, board-dependent
    virtual bool startAp(const WifiConfig& cfg) = 0;
    virtual bool startSta(const WifiConfig& cfg) = 0;
    virtual void stop() = 0;
    virtual String getIpAddress() = 0;
};
