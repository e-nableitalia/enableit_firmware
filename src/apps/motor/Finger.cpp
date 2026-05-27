
#include "Finger.h"

namespace motor {

void Finger::init(enableit::PwmServoMotor* servo, int maxOpen, int maxClosed) {
    _servo     = servo;
    _maxOpen   = maxOpen;
    _maxClosed = maxClosed;
    _pct       = 0;
}

// map() gestisce correttamente anche il caso maxOpen > maxClosed (invertito)
int Finger::_toRaw(int pct) const {
    return map(pct, 0, 100, _maxOpen, _maxClosed);
}

void Finger::setRelativePosition(int pct) {
    if (!_servo) return;
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    _pct = pct;
    _servo->setPosition(_toRaw(pct));
}

void Finger::open() {
    setRelativePosition(0);
}

void Finger::close() {
    setRelativePosition(100);
}

void Finger::setMaxOpen(int maxOpen) {
    _maxOpen = maxOpen;
}

void Finger::setMaxClosed(int maxClosed) {
    _maxClosed = maxClosed;
}

void Finger::setRange(int maxOpen, int maxClosed) {
    _maxOpen  = maxOpen;
    _maxClosed = maxClosed;
}

int Finger::getRelativePosition() const {
    return _pct;
}

int Finger::getRawPosition() const {
    if (!_servo) return -1;
    return _servo->getPosition();
}

} // namespace motor
