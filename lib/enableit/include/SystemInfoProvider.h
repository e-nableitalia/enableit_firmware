#pragma once

#include <Arduino.h>
#include <Esp.h>
#include <Board.h>
#include <Config.h>
#include <ArduinoJson.h>

#define FWREV "1.0." __DATE__ "." __TIME__

namespace enableit
{
    class SystemInfoProvider
    {
    public:
        void init(Board &board, Config &config);
        void dump() const;
        void serialize(String& out) const;
        const JsonDocument& view() const;
    private:
        JsonDocument systemInfo;
        boolean initialized = false;
        esp_chip_info_t chip_info;
        String chipId;
        String flashInfo;
        String coresInfo;
        String sdkChipInfo;
    };

    extern SystemInfoProvider systeminfo;

} // namespace enableit