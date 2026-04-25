#pragma once
#include <BleCommandDispatcher.h>
#include <BoardManager.h>
#include <CommandParser.h>

/**
 * @class OtaCommandHandler
 * @brief Handles OTA (Over-The-Air) related BLE commands using the BleV1CommandDispatcher interface.
 *
 * This class provides command handling for OTA operations such as status, start, stop, and URL management.
 * It integrates with the BLE command dispatcher and uses a command parser for command routing.
 *
 * @constructor OtaCommandHandler(String otaAppName, String uuid, uint32_t properties)
 *      Constructs an OTA command handler for a given OTA application name and BLE characteristic UUID.
 * @destructor ~OtaCommandHandler()
 *      Destructor for the OTA command handler.
 * @fn void handle(const String& cmd, String& response) override
 *      Handles incoming BLE commands and writes the response.
 * @fn void cmdStatus()
 *      Handles the "status" OTA command, providing current OTA status information.
 * @fn void cmdStart()
 *      Handles the "start" OTA command, initiating the OTA process.
 * @fn void cmdStop()
 *      Handles the "stop" OTA command, terminating the OTA process.
 * @fn void cmdUrl()
 *      Handles the "url" OTA command, managing the OTA update URL.
 * @fn virtual const char* name() const override
 *      Returns the name identifier for this command handler.
 *
 * @private
 * String otaAppName_         The name of the OTA application being managed.
 * String previousAppName_    The name of the previously active application.
 * CommandParser<OtaCommandHandler> parser_  Command parser for routing commands.
 * String currentResponse_    Stores the current response to be sent.
 */
class OtaCommandHandler : public enableit::BleV1CommandDispatcher {
public:
    OtaCommandHandler(String otaAppName, const char* uuid, uint32_t properties);
    ~OtaCommandHandler();
    void handle(const String& cmd, String& response) override;

    // Command implementations
    void cmdStatus();
    void cmdStart();
    void cmdStop();
    void cmdUrl();

    virtual const char* name() const override {
        return "ble_v1_ota_command_handler";
    }
private:
    String otaAppName_;
    String previousAppName_;
    CommandParser<OtaCommandHandler> parser_;
    String currentResponse_;
};

/**
 * @def ENABLE_OTA_COMMAND_HANDLER(varname, appName, uuid)
 * @brief Macro to instantiate an OtaCommandHandler with standard BLE properties (READ, WRITE, NOTIFY).
 *
 * @param varname   The variable name for the OtaCommandHandler instance.
 * @param appName   The name of the OTA application.
 * @param uuid      The BLE characteristic UUID.
 */
#define ENABLE_OTA_COMMAND_HANDLER(varname, appName, uuid) \
    OtaCommandHandler varname(appName, uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
