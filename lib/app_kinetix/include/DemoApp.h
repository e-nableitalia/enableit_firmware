#pragma once
#include <BoardApp.h>
#include "Hand.h"
#include "HandMovementFactory.h"
#include "Settings.h"
#include "Sequence.h"

class DemoApp : public enableit::BoardApp {
public:
    void enter() override;
    void process() override;
    void leave() override;
    const char* name() override { return "demo"; }
private:
    Hand hand_;
    HandMovementFactory* hmf_ = nullptr;
    Sequence* seq_ = nullptr;
};