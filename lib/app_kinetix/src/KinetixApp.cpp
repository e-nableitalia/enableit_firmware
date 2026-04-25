#include "KinetixApp.h"

#include <RuntimeManager.h>
#include <SystemInfoProvider.h>
#include <OtaCommandHandler.h>

#include "SettingsCommandDispatcher.h"
#include "SystemConfigCommandDispatcher.h"
#include "Hand.h"

#define CONNECTED_DISPLAY_LINE 2
#define SENSOR_DISPLAY_LINE 3
#define MOVEMENT_DISPLAY_LINE 4

#define BLE_DEVICE_NAME "KinetiX"

#define SERVICE_UUID "89d60870-9908-4472-8f8c-e5b3e6573cd1"

#define OTA_CHARACTERISTIC_UUID "3168e56f-6ea1-420d-98f8-08a3b34afc9b"

#define SYSTEM_INFO_SECTION "options"

BOARDAPP_INSTANCE(KinetixApp);

// global instances
Hand hand_;
int offset = 380;
int threshold = 2;

void KinetixApp::enter()
{
    log_i("Entering Kinetix App state");

// add misc info in system info
#ifdef CORE_DEBUG_LEVEL
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, String("CORE_DEBUG_LEVEL"), String(CORE_DEBUG_LEVEL));
#endif

#ifdef LEFT_HAND
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "HAND", "LEFT");
#else
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "HAND", "RIGHT");
#endif

    log_i("Inizializing BLE service and characteristics");

    enableit::runtime.enableBle(BLE_DEVICE_NAME, SERVICE_UUID);

    log_i("Registering Kinetix movement, setting and systemConfig features");
    movementCommandDispatcher_ = new MovementCommandDispatcher(&hand_);
    enableit::runtime.registerBleCommandDispatcher(movementCommandDispatcher_);
    enableit::runtime.registerBleCommandDispatcher(new SettingsCommandDispatcher(&settings_));
    enableit::runtime.registerBleCommandDispatcher(new SystemConfigCommandDispatcher());
    enableit::runtime.registerBleCommandDispatcher(
        new OtaCommandHandler(
            APP_OTAUPDATE,
            OTA_CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
        )
    );

    // start ble operations
    log_i("Starting BLE");
    enableit::runtime.startBle();

#ifdef WITH_SENSOR
    // Create sensor processor with initial offset and threshold
    offset = settings_.getInt("i_1", 380);
    threshold = settings_.getInt("i_2", 2);
    sensorProcessor_ = new RealSensorProcessor(offset, threshold);
    // Optionally, set a callback to handle sensor updates
    sensorProcessor_->setUpdateCallback([](int position, void *ctx)
                                        {
            // Handle sensor position update
            log_i("Sensor position updated: %d", position);
            // You can add code here to move the hand or perform other actions
            // display position, offset and threshold for debugging
            enableit::board.getDisplay().setLine(SENSOR_DISPLAY_LINE, "Sensor: " + String(position) + " (" + String(offset) + ", " + String(threshold) + ")");
            log_i("Sensor position: %d", position);
            // high limit should be adjustable saved in config
            hand_.moveRelative(map(position, 0, 100, 0, 180)); }, nullptr);
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "SENSOR");
#else
    sensorProcessor_ = new MockSensorProcessor();
#endif

    String boardName = enableit::board.name();

    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "BOARD", boardName);

    lastBleConnected = enableit::runtime.bleConnected();

    enableit::board.getDisplay().setTextSize(2);
    enableit::board.getDisplay().setTitle("KinetiX-" + boardName);
    enableit::board.getDisplay().setTextSize(1);
    enableit::board.getDisplay().setLine(CONNECTED_DISPLAY_LINE, "BLE: " + String(lastBleConnected ? "Connected" : "Disconnected"));
    enableit::board.getDisplay().setLine(SENSOR_DISPLAY_LINE, "Sensor: " + String(sensorProcessor_->name()));
}

void KinetixApp::leave()
{
    log_i("Leaving Kinetix App state");

    // Disable BLE removed to avoid interference with other apps: OtaApp for example
    //enableit::runtime.disableBle();

    // Clean up sensor processor
    if (sensorProcessor_)
    {
        delete sensorProcessor_;
        sensorProcessor_ = nullptr;
    }
    // Remove custom info from system info
    enableit::systeminfo.clearCustomInfo(SYSTEM_INFO_SECTION);
}

void KinetixApp::process()
{
    if (movementCommandDispatcher_)
        movementCommandDispatcher_->run();
    
    if (sensorProcessor_)
        sensorProcessor_->run();
    
    hand_.run();

    bool currentBleConnected = enableit::runtime.bleConnected();

    if (currentBleConnected != lastBleConnected) {
        // update on BLE state change only
        enableit::board.getDisplay().setLine(CONNECTED_DISPLAY_LINE, "BLE: " + String(currentBleConnected ? "Connected" : "Disconnected"));
        lastBleConnected = currentBleConnected;
    }
}

const char *KinetixApp::name()
{
    return "kinetix";
}
