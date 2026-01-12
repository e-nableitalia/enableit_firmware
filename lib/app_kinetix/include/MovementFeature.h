#pragma once
#include "Feature.h"
#include "Hand.h"
#include "Sequence.h"
#include "HandMovementFactory.h"
#include <Arduino.h>

class MovementFeature : public Feature {
public:
    MovementFeature(Hand* hand);
    const char* name() const override { return "movement"; }
    void handleV1(const String& cmd, String& response) override;
    void handleV2(const JsonObjectConst& msg, JsonObject& response) override;
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
