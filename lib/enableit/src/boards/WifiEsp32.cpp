#include <boards/WifiEsp32.h>

namespace enableit {

bool WifiEsp32::startAp(const WifiConfig& cfg) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg.ssid, cfg.password);
    return true;
}

bool WifiEsp32::startSta(const WifiConfig& cfg) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.ssid, cfg.password);
    return true;
}

void WifiEsp32::stop() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
}

} // namespace enableit
