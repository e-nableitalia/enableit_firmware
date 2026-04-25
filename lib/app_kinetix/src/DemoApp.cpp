#include "DemoApp.h"
#include <RuntimeManager.h>
#include <SystemInfoProvider.h>
#include <SystemConfigCommandDispatcher.h>
#include <enableit.h>

#define SYSTEM_INFO_SECTION "options"

#define BLE_DEVICE_NAME "KinetiX"
#define SERVICE_UUID "89d60870-9908-4472-8f8c-e5b3e6573cd1"

BOARDAPP_INSTANCE(DemoApp);

void DemoApp::enter()
{
    // --- Common init (similar to KinetixApp) ---
    Serial.println("Entering DemoApp");

#ifdef CORE_DEBUG_LEVEL
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "CORE_DEBUG_LEVEL=" + String(CORE_DEBUG_LEVEL));
#endif

#ifdef LEFT_HAND
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "LEFT_HAND");
#else
    enableit::systeminfo.addCustomInfo(SYSTEM_INFO_SECTION, "RIGHT_HAND");
#endif

    // --- BLE init as in KinetixApp ---
    log_i("Inizializing BLE service and characteristics");
    enableit::runtime.enableBle(BLE_DEVICE_NAME, SERVICE_UUID);

    log_i("Registering DemoApp systemConfig features");
    enableit::runtime.registerBleCommandDispatcher(new SystemConfigCommandDispatcher());

    log_i("Starting BLE");
    enableit::runtime.startBle();
    
    hmf_ = new HandMovementFactory(&hand_);

#ifndef NEEDSEQ
    // Initialization sequence, do it just once
    seq_ = new Sequence(1); // this sequence runs just once
    seq_->addMovement(hmf_->five());
    seq_->addMovement(hmf_->half());
    seq_->addMovement(hmf_->five());
    log_i("Running init sequence");
    seq_->start();
#endif

    // --- Custom DemoApp init ---
    seq_ = new Sequence(0); // repeat forever
    seq_->addMovement(hmf_->openPinch(), 4000);
    seq_->addMovement(hmf_->one());
    seq_->addMovement(hmf_->two());
    seq_->addMovement(hmf_->three());
    seq_->addMovement(hmf_->four());
    seq_->addMovement(hmf_->five());
    seq_->start();
}

void DemoApp::process()
{
    if (seq_)
        seq_->run();
    hand_.run();
}

void DemoApp::leave()
{
    // --- Common deinit (similar to KinetixApp) ---
    Serial.println("Leaving DemoApp");
    enableit::systeminfo.clearCustomInfo(SYSTEM_INFO_SECTION);

    // --- Custom DemoApp deinit ---
    if (seq_)
    {
        delete seq_;
        seq_ = nullptr;
    }
    if (hmf_)
    {
        delete hmf_;
        hmf_ = nullptr;
    }
}