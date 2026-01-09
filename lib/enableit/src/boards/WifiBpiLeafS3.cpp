#include <boards/WifiBpiLeafS3.h>
#include <Arduino.h>

namespace enableit {

bool WifiBpiLeafS3::startAp(const WifiConfig&) {
    // AP mode not supported on BPI_LEAF_S3
    // Optionally log or warn
    #pragma message("WiFi AP mode is not supported on BPI_LEAF_S3")
    return false;
}

bool WifiBpiLeafS3::startSta(const WifiConfig& cfg) {
    // Do not call WiFi.mode
    WiFi.begin(cfg.ssid, cfg.password);
    return true;
}

void WifiBpiLeafS3::stop() {
    WiFi.disconnect();
    // WiFi.mode(WIFI_OFF) not supported
    #pragma message("WiFi OFF mode is not supported on BPI_LEAF_S3")
}

} // namespace enableit
