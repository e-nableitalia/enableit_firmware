
#include <cmath>
#include "Finger.h"

namespace motor {

void Finger::init(enableit::PwmServoMotor* servo, int maxOpen, int maxClosed) {
    _servo          = servo;
    _maxOpen        = maxOpen;
    _maxClosed      = maxClosed;
    _currentPct     = 0.0f;
    _targetPct      = 0;
    _currentSpeed   = 0.0f;
    _maxSpeed       = 120.0f; // 120% per secondo
    _maxAccel       = 300.0f; // 300% per secondo^2
    _lastUpdateTime = millis();
    _lastRawPos     = _toRaw(0);
    
    // Posizionamento istantaneo a 0 all'inizializzazione per evitare scatti
    if (_servo) {
        _servo->setPosition(_lastRawPos);
    }
}

// map() gestisce correttamente anche il caso maxOpen > maxClosed (invertito)
int Finger::_toRaw(int pct) const {
    return map(pct, 0, 100, _maxOpen, _maxClosed);
}

void Finger::setRelativePosition(int pct) {
    if (!_servo) return;
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    _targetPct = pct;
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

void Finger::setSpeed(float maxSpeed) {
    if (maxSpeed > 0) {
        _maxSpeed = maxSpeed;
    }
}

void Finger::setAcceleration(float maxAccel) {
    if (maxAccel > 0) {
        _maxAccel = maxAccel;
    }
}

void Finger::poll() {
    if (!_servo) return;

    unsigned long now = millis();
    unsigned long dt_ms = now - _lastUpdateTime;

    if (dt_ms == 0) return; // Ritorna senza aggiornare _lastUpdateTime
    _lastUpdateTime = now;

    float dt = dt_ms / 1000.0f; // Converti in secondi

    // 1. Velocità desiderata proporzionale all'errore (Kp = 8.0f)
    float error = (float)_targetPct - _currentPct;
    float desiredSpeed = error * 8.0f;

    // 2. Limitazione della velocità desiderata alla velocità massima consentita
    if (desiredSpeed > _maxSpeed) {
        desiredSpeed = _maxSpeed;
    } else if (desiredSpeed < -_maxSpeed) {
        desiredSpeed = -_maxSpeed;
    }

    // 3. Limitazione dell'accelerazione (tasso di variazione della velocità)
    float speedError = desiredSpeed - _currentSpeed;
    float maxSpeedChange = _maxAccel * dt;

    if (speedError > maxSpeedChange) {
        _currentSpeed += maxSpeedChange;
    } else if (speedError < -maxSpeedChange) {
        _currentSpeed -= maxSpeedChange;
    } else {
        _currentSpeed = desiredSpeed;
    }

    // 4. Integrazione della posizione
    _currentPct += _currentSpeed * dt;

    // Vincolo della posizione tra 0 e 100
    if (_currentPct < 0.0f) {
        _currentPct = 0.0f;
        _currentSpeed = 0.0f;
    } else if (_currentPct > 100.0f) {
        _currentPct = 100.0f;
        _currentSpeed = 0.0f;
    }

    // Aggancio preciso se siamo molto vicini al target
    if (fabs(error) < 0.1f && fabs(_currentSpeed) < 1.0f) {
        _currentPct = (float)_targetPct;
        _currentSpeed = 0.0f;
    }

    int rawPos = _toRaw((int)round(_currentPct));
    if (rawPos != _lastRawPos) {
        _servo->setPosition(rawPos);
        _lastRawPos = rawPos;
    }
}

int Finger::getRelativePosition() const {
    return (int)round(_currentPct);
}

int Finger::getRawPosition() const {
    if (!_servo) return -1;
    return _servo->getPosition();
}

} // namespace motor
