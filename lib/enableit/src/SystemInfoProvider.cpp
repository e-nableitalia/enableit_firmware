#include "SystemInfoProvider.h"
#include <Esp.h>
#include <ArduinoJson.h>

namespace enableit {

void SystemInfoProvider::init(Board &board, Config &config)
{
	initialized = true;
	systemInfo["chip"] = ESP.getChipModel();
	chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
	chipId.toUpperCase();
	systemInfo["chip_id"] = chipId;
	systemInfo["cpu_mhz"] = ESP.getCpuFreqMHz();
	systemInfo["psram"] = ESP.getPsramSize();

	esp_chip_info(&chip_info);

	flashInfo = String(spi_flash_get_chip_size() / (1024 * 1024)) + "MB ";
	flashInfo += (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external";
	flashInfo += " flash, Memory " + String(ESP.getPsramSize());
	systemInfo["flash"] = flashInfo;
	systemInfo["device_id"] = config.deviceid;
	systemInfo["mdns"] = "esp32";

	coresInfo = String(chip_info.cores) + " cores Wifi";
	if (chip_info.features & CHIP_FEATURE_BT)
		coresInfo += "/BT";
	if (chip_info.features & CHIP_FEATURE_BLE)
		coresInfo += "/BLE";
	coresInfo += ", " + String(ESP.getCpuFreqMHz()) + " Mhz";
	systemInfo["cores_info"] = coresInfo;

	systemInfo["silicon_revision"] = chip_info.revision;

	sdkChipInfo = "SDK: " + String(ESP.getSdkVersion()) +
						 ", Chip: " + String(ESP.getChipModel()) +
						 ", Chip id: " + chipId;
	systemInfo["sdk_chip_info"] = sdkChipInfo;

	systemInfo["fw_checksum"] = ESP.getSketchMD5();

#ifdef FWREV
	systemInfo["fw_rev"] = FWREV;
#endif
#ifdef GIT_REV
	systemInfo["git_rev"] = GIT_REV;
#endif
    dump();
}

void SystemInfoProvider::dump() const
{
	// Log all build information
	log_i("System Info:");
	log_i("  Chip Model: %s", ESP.getChipModel());
	log_i("  Chip ID: %s", chipId.c_str());
	log_i("  CPU MHz: %d", ESP.getCpuFreqMHz());
	log_i("  PSRAM Size: %d", ESP.getPsramSize());
	log_i("  Flash Info: %s", flashInfo.c_str());
	log_i("  Device ID: %s", config.deviceid.c_str());
	log_i("  mDNS: esp32");
	log_i("  Cores Info: %s", coresInfo.c_str());
	log_i("  Silicon Revision: %d", chip_info.revision);
	log_i("  SDK Chip Info: %s", sdkChipInfo.c_str());
	log_i("  FW Checksum: %s", ESP.getSketchMD5().c_str());
#ifdef FWREV
	log_i("  FW Rev: %s", FWREV);
#endif
#ifdef GIT_REV
	log_i("  Git Rev: %s", GIT_REV);
#endif
}

void SystemInfoProvider::serialize(String& out) const
{
	if (!initialized)
	{
		return;
	}   
	serializeJson(systemInfo, out);
}

const JsonDocument& SystemInfoProvider::view() const
{
	return systemInfo;
}

void SystemInfoProvider::addCustomInfo(const String& section, const String& key, const JsonVariantConst& value)
{
	if (!initialized)
	{
		return;
	}
	JsonObject root = systemInfo.as<JsonObject>();
	JsonObject sectionObj;
	if (root.containsKey(section))
	{
		sectionObj = root[section].as<JsonObject>();
	}
	else
	{
		sectionObj = root.createNestedObject(section);
	}
	sectionObj[key] = value;
}

void SystemInfoProvider::addCustomInfo(const String& section, const String& key)
{
	if (!initialized)
	{
		return;
	}
	JsonObject root = systemInfo.as<JsonObject>();
	JsonObject infoObj;
	if (root.containsKey(section))
	{
		infoObj = root[section].as<JsonObject>();
	}
	else
	{
		infoObj = root.createNestedObject(section);
	}
	infoObj[key];
}

void SystemInfoProvider::removeCustomInfo(const String& section, const String& key)
{
	if (!initialized)
	{
		return;
	}
	JsonObject root = systemInfo.as<JsonObject>();
	if (root.containsKey(section))
	{
		JsonObject sectionObj = root[section].as<JsonObject>();
		if (sectionObj.containsKey(key))
		{
			sectionObj.remove(key);
		}
	}
}

void SystemInfoProvider::clearCustomInfo(const String& section)
{
	if (!initialized)
	{
		return;
	}
	JsonObject root = systemInfo.as<JsonObject>();
	if (root.containsKey(section))
	{
		root.remove(section);
	}
}

SystemInfoProvider systeminfo;

} // namespace enableit
