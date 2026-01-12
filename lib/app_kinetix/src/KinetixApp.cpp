#include "KinetixApp.h"
#include <RuntimeManager.h>
#include "MovementFeature.h"
#include "SettingFeature.h"
#include "SystemConfigFeature.h"
#include "Hand.h"

#define CONNECTED_DISPLAY_LINE 0
#define SENSOR_DISPLAY_LINE 1
#define MOVEMENT_DISPLAY_LINE 2

// global instances
Hand hand_;
int offset = 380;
int threshold = 2;

void KinetixApp::enter() {
    log_i("Entering Kinetix App state");

    log_i("Registering Kinetix movement, setting and systemConfig features");
    movementFeature_ = new MovementFeature(&hand_);
    enableit::runtime.registerFeature(movementFeature_);
    enableit::runtime.registerFeature(new SettingFeature(&settings_));
    enableit::runtime.registerFeature(new SystemConfigFeature());

    // Create sensor processor with initial offset and threshold
    offset = settings_.getInt("i_1", 380);
    threshold = settings_.getInt("i_2", 2);
    sensorProcessor_ = new RealSensorProcessor(offset, threshold);
    // Optionally, set a callback to handle sensor updates
    sensorProcessor_->setUpdateCallback([](int position, void* ctx) {
        // Handle sensor position update
        log_i("Sensor position updated: %d", position);
        // You can add code here to move the hand or perform other actions
        // display position, offset and threshold for debugging
        enableit::board.getDisplay().setLine(SENSOR_DISPLAY_LINE, "Sensor: " + String(position) + " (" + String(offset) + ", " + String(threshold) + ")");
        log_i("Sensor position: %d", position);
        // high limit should be adjustable saved in config
        hand_.moveRelative(map(position, 0, 100, 0, 180));        
    }, nullptr);
}

void KinetixApp::leave() {
    log_i("Leaving Kinetix App state");
    // Unregister features
    auto movement = enableit::runtime.getFeature("movement");
    if (movement) {
        enableit::runtime.unregisterFeature("movement");
        delete movement;
    }
    auto setting = enableit::runtime.getFeature("setting");
    if (setting) {
        enableit::runtime.unregisterFeature("setting");
        delete setting;
    }
    auto systemConfig = enableit::runtime.getFeature("systemConfig");
    if (systemConfig) {
        enableit::runtime.unregisterFeature("systemConfig");
        delete systemConfig;
    }
}

void KinetixApp::process() {
    movementFeature_->run();
    sensorProcessor_->run();
    hand_.run();
}

const char* KinetixApp::name() {
    return "kinetix";
}
