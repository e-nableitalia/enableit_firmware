#include <cstring>
#include <ArduinoJson.h>
#include <Board.h>
#include "MovementCommandDispatcher.h"

#define MOVEMENT_DISPLAY_LINE 2


MovementCommandDispatcher::MovementCommandDispatcher(Hand* hand)
    : enableit::BleV1CommandDispatcher( MOVEMENT_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE ),
      hand_(hand)
{
    hmf_ = new HandMovementFactory(hand_);
}

void MovementCommandDispatcher::handle(const String& cmd, String& response) {
    // V1: movement:calibration, movement:scratch, movement:come, movement:<name>
    log_i("Handling V1 movement command: %s", cmd.c_str());
    String line = "Movement: " + cmd;
    enableit::board.getDisplay().setLine(MOVEMENT_DISPLAY_LINE, line.c_str());
    startMovement(cmd, response);
}

void MovementCommandDispatcher::run() {
    if (seq_ != nullptr) {
        seq_->run();
    }

    hand_->run();
}

bool MovementCommandDispatcher::isIdle() const {
    if (handMovement_ != nullptr) {
        return handMovement_->isFinished();
    }
    return true;
}

void MovementCommandDispatcher::startMovement(String cmd, String &response) {
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
        //response = "OK: calibration started";
        return;
    }
    if (cmd == "scratch") {
        hand_->stop();
        scratch();
        //response = "OK: scratch started";
        return;
    }
    if (cmd == "come") {
        hand_->stop();
        come();
        //response = "OK: come started";
        return;
    }
    
    if (cmd == "demo") {
    	hand_->stop();
    	demo();
    	return;
	}
    HandMovement* newHandMovement = hmf_->getByName(cmd.c_str());
    if (newHandMovement != nullptr) {
        hand_->stop();
        handMovement_ = newHandMovement;
        handMovement_->start();
    }
    //response = "OK: movement started";
}

void MovementCommandDispatcher::calibration() {
    hand_->setCalibration(true);
    HandMovementFactory* calibrationHmf = new HandMovementFactory(hand_);
    seq_ = new Sequence(0);
    seq_->addMovement(calibrationHmf->five(), 5000);
    seq_->addMovement(calibrationHmf->fist(), 1500);
    seq_->start();
    delete calibrationHmf;
}

void MovementCommandDispatcher::scratch() {
    HandMovementFactory* hmf = new HandMovementFactory(hand_);
    seq_ = new Sequence(0);
    seq_->addMovement(hmf->scratchOpen(), 600);
    seq_->addMovement(hmf->scratchClose(), 600);
    seq_->start();
    delete hmf;
}

void MovementCommandDispatcher::come() {
    HandMovementFactory* hmf = new HandMovementFactory(hand_);
    seq_ = new Sequence(5);
    seq_->addMovement(hmf->comeOpen(), 500);
    seq_->addMovement(hmf->comeClose(), 500);
    seq_->start();
    delete hmf;
}

void MovementCommandDispatcher::demo() {
  log_i("Starting demo sequence");
  HandMovementFactory *hmf = new HandMovementFactory(hand_);
  seq_ = new Sequence(0);
  int delai = 1200;
  seq_->addMovement(hmf->one(), delai);
  seq_->addMovement(hmf->two(), delai);
  seq_->addMovement(hmf->three(), delai);
  seq_->addMovement(hmf->four(), delai);
  seq_->addMovement(hmf->five(), delai);
  seq_->addMovement(hmf->fist(), delai);
  seq_->addMovement(hmf->rock(), delai);
  seq_->addMovement(hmf->love(), delai);
  seq_->start();
}