#include "KinetixApp.h"
#include <RuntimeManager.h>

#include "SettingsCommandDispatcher.h"
#include "SystemConfigCommandDispatcher.h"
#include <SystemInfoProvider.h>
#include "Hand.h"

#define CONNECTED_DISPLAY_LINE 0
#define SENSOR_DISPLAY_LINE 1
#define MOVEMENT_DISPLAY_LINE 2

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
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "CORE_DEBUG_LEVEL=" + String(CORE_DEBUG_LEVEL));
#endif

#ifdef LEFT_HAND
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "LEFT_HAND");
#else
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "RIGHT_HAND");
#endif

    log_i("Inizializing BLE service and characteristics");

    enableit::runtime.enableBle(BLE_DEVICE_NAME, SERVICE_UUID);

    log_i("Registering Kinetix movement, setting and systemConfig features");
    movementCommandDispatcher_ = new MovementCommandDispatcher(&hand_);
    enableit::runtime.registerBleCommandDispatcher(movementCommandDispatcher_);
    enableit::runtime.registerBleCommandDispatcher(new SettingsCommandDispatcher(&settings_));
    enableit::runtime.registerBleCommandDispatcher(new SystemConfigCommandDispatcher());

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
}

void KinetixApp::leave()
{
    log_i("Leaving Kinetix App state");

    // Disable BLE
    enableit::runtime.disableBle();

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
}

const char *KinetixApp::name()
{
    return "kinetix";
}
