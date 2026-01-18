#pragma once
#include "Feature.h"
#include "Hand.h"
#include "Sequence.h"
#include "HandMovementFactory.h"
#include "BleCommandDispatcher.h"
#include <Arduino.h>

constexpr char MOVEMENT_CHARACTERISTIC_UUID[] = "39dea685-a63e-44b2-8819-9a202581f8fe";

class MovementCommandDispatcher : public enableit::BleV1CommandDispatcher {
public:
    MovementCommandDispatcher(Hand* hand);
    const char* name() const override { return "movement"; }
    void handle(const String& cmd, String& response) override;
    void run();
    bool isIdle() const;
    void calibration();
    void scratch();
    void come();
private:
    void startMovement(String cmd, String &response);

    Hand* hand_;
    Sequence* seq_ = nullptr;
    HandMovement* handMovement_ = nullptr;
    HandMovementFactory* hmf_ = nullptr;
};
