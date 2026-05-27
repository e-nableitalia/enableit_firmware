#pragma once

#include <PwmServoMotor.h>


namespace motor {

/**
 * Finger — astrazione di un dito azionato da un servo PWM.
 *
 * Il dito ha una posizione "tutto aperto" (0%) e "tutto chiuso" (100%).
 * maxOpen e maxClosed sono i valori raw del servo (0-180) che corrispondono
 * rispettivamente a 0% e 100% di chiusura.
 * setRelativePosition(int pct) accetta valori da 0 (aperto) a 100 (chiuso).
 */
class Finger {
public:
    Finger() = default;

    void init(enableit::PwmServoMotor* servo, int maxOpen, int maxClosed);

    void setRelativePosition(int pct); // 0 = aperto, 100 = chiuso
    void open();
    void close();

    void setMaxOpen(int maxOpen);
    void setMaxClosed(int maxClosed);
    void setRange(int maxOpen, int maxClosed);

    int  getRelativePosition() const;   // ritorna 0-100
    int  getRawPosition() const;        // ritorna la posizione raw del servo
    int  getMaxOpen()   const { return _maxOpen; }
    int  getMaxClosed() const { return _maxClosed; }
    // Se maxOpen > maxClosed il motore è montato al contrario
    bool isInverted()   const { return _maxOpen > _maxClosed; }

private:
    enableit::PwmServoMotor* _servo    = nullptr;
    int  _maxOpen   = 0;
    int  _maxClosed = 180;
    int  _pct       = 0;

    int _toRaw(int pct) const;
};

} // namespace motor
