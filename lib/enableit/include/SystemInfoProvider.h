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

        // Add a key-value pair to a custom section (creates section if not present)
        void addCustomInfo(const String& section, const String& key, const String& value);
        void addCustomInfo(const String& key, const String& value);

        // Remove a key from a custom section
        void removeCustomInfo(const String& section, const String& key);
        void clearCustomInfo(const String& section);

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