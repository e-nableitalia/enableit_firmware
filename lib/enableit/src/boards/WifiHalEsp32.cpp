#include <boards/WifiHalEsp32.h>

bool WifiHalEsp32::startAp(const WifiConfig& cfg) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg.ssid, cfg.password);
    return true;
}

bool WifiHalEsp32::startSta(const WifiConfig& cfg) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.ssid, cfg.password);
    return true;
}

void WifiHalEsp32::stop() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}
