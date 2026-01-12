#include <cstring>
#include <ArduinoJson.h>
#include <Board.h>
#include "MovementFeature.h"

#define MOVEMENT_DISPLAY_LINE 2

MovementFeature::MovementFeature(Hand* hand)
    : hand_(hand)
{
    hmf_ = new HandMovementFactory(hand_);
}

void MovementFeature::handleV1(const String& cmd, String& response) {
    // V1: movement:calibration, movement:scratch, movement:come, movement:<name>
    log_i("Handling V1 movement command: %s", cmd.c_str());
    String line = "Movement: " + cmd;
    enableit::board.getDisplay().setLine(MOVEMENT_DISPLAY_LINE, line.c_str());
    startMovement(cmd, response);
}

void MovementFeature::handleV2(const JsonObjectConst& msg, JsonObject& response) {
    // V2: { target: "movement", action: "start", movement: "name" }
    log_i("Handling V2 movement command");
    response["status"] = "error";
    response["error"] = "Invalid command, v2 movement commands are not supported yet";
}

void MovementFeature::run() {
    if (handMovement_ != nullptr) {
        handMovement_->run();
    }
    if (seq_ != nullptr) {
        seq_->run();
    }
}

bool MovementFeature::isIdle() const {
    if (handMovement_ != nullptr) {
        return handMovement_->isFinished();
    }
    return true;
}

void MovementFeature::startMovement(String cmd, String &response) {
   hand_->setCalibration(false);
    if (handMovement_ != nullptr) {
        delete handMovement_;
        handMovement_ = nullptr;
    }
    if (seq_ != nullptr) {
        delete seq_;
        seq_ = nullptr;
    }
    if (cmd == "calibration") {
        hand_->stop();
        calibration();
        response = "OK: calibration started";
        return;
    }
    if (cmd == "scratch") {
        hand_->stop();
        scratch();
        response = "OK: scratch started";
        return;
    }
    if (cmd == "come") {
        hand_->stop();
        come();
        response = "OK: come started";
        return;
    }
    HandMovement* newHandMovement = hmf_->getByName(cmd.c_str());
    if (newHandMovement != nullptr) {
        hand_->stop();
        handMovement_ = newHandMovement;
        handMovement_->start();
    }
    response = "OK: movement started";
}

void MovementFeature::calibration() {
    hand_->setCalibration(true);
    HandMovementFactory* calibrationHmf = new HandMovementFactory(hand_);
    seq_ = new Sequence(0);
    seq_->addMovement(calibrationHmf->five(), 5000);
    seq_->addMovement(calibrationHmf->fist(), 1500);
    seq_->start();
    delete calibrationHmf;
}

void MovementFeature::scratch() {
    HandMovementFactory* hmf = new HandMovementFactory(hand_);
    seq_ = new Sequence(0);
    seq_->addMovement(hmf->scratchOpen(), 600);
    seq_->addMovement(hmf->scratchClose(), 600);
    seq_->start();
    delete hmf;
}

void MovementFeature::come() {
    HandMovementFactory* hmf = new HandMovementFactory(hand_);
    seq_ = new Sequence(5);
    seq_->addMovement(hmf->comeOpen(), 500);
    seq_->addMovement(hmf->comeClose(), 500);
    seq_->start();
    delete hmf;
}
