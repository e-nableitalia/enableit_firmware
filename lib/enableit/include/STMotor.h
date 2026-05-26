// STMotor: Waveshare ST series servo (ST3215, SMS_STS family)
// Implements enableit::Motor abstract interface
// Author: A.Navatta

#pragma once

#include <Arduino.h>
#include <SCServo.h>
#include <Motor.h>

// ── ST3215 hardware constants ─────────────────────────────────────────────────────
#define ST_POS_MIN           0        // minimum encoder position
#define ST_POS_MAX           4095     // maximum encoder position (12-bit)
#define ST_SPEED_MAX         3400     // maximum speed [steps/s]
#define ST_ACC_MAX           254      // maximum acceleration register value
#define ST_CURRENT_UNIT_UA   6500     // current register LSB [µA] (6.5 mA per step)

namespace enableit {

class STMotor : public SMS_STS, public Motor {
public:
    STMotor();

    // Bus-level init (shared across all STMotor instances on the same UART)
    // Must be called before begin()
    static void static_init(int sRX, int sTX);

    // Per-servo setup — stores id, links pSerial
    void init(uint8_t servoId);

    // ── Motor abstract interface ──────────────────────────────────────────────
    bool begin()  override;   // calls init() with stored id; requires static_init() done
    void end()    override;   // disables torque
    void stop()   override;   // disables torque (coast)
    void brake()  override;   // holds at current position
    void enable()  override;  // restore torque
    void disable() override;  // remove torque

    // speed = -1 → use last configured speed (or 1000 if never set)
    // forward: moves toward max position (4095)
    // reverse: moves toward min position (0)
    bool forward(int speed = -1, int acc = 0) override;
    bool reverse(int speed = -1, int acc = 0) override;

    bool setSpeed(int value) override;
    bool setPosition(int value, int speed = -1, int acc = 0) override;
    bool move(int delta, int speed = -1, int acc = 0) override;

    MotorCapabilities getCapabilities() const override;
    MotorState        getState()        const override;
    MotorTelemetry    getTelemetry()    const override;
    uint32_t          getError()        const override;
    const char*       getType()         const override { return "STMotor"; }

    int getPosition() override;
    int getSpeed()    override;
    int getCurrent()  override;

    // ── Extended servo-specific methods ─────────────────────────────────────
    bool moveTo(int position, int speed = -1, int acc = 0);
    void poll();

private:
    // ── Value-mode conversion helpers ─────────────────────────────────────────
    // Integer remap: maps x in [inMin, inMax] → [outMin, outMax]
    static inline int _remap(int x, int inMin, int inMax, int outMin, int outMax) {
        if (inMax == inMin) return outMin;
        return outMin + (int)((long)(x - inMin) * (outMax - outMin) / (inMax - inMin));
    }
    // Speed: normalized [0,1000] ⇄ effective [0, ST_SPEED_MAX]
    static inline int _normSpeedToEff(int n) { return _remap(n, 0, 1000, 0, ST_SPEED_MAX); }
    static inline int _effSpeedToNorm(int e) { return _remap(e, 0, ST_SPEED_MAX, 0, 1000); }
    // Position: normalized [0,1000] ⇄ effective [0, ST_POS_MAX]
    static inline int _normPosToEff(int n)   { return _remap(n, 0, 1000, ST_POS_MIN, ST_POS_MAX); }
    static inline int _effPosToNorm(int e)   { return _remap(e, ST_POS_MIN, ST_POS_MAX, 0, 1000); }
    // Acceleration: normalized [0,1000] ⇄ effective [0, ST_ACC_MAX]
    static inline int _normAccToEff(int n)   { return _remap(n, 0, 1000, 0, ST_ACC_MAX); }
    // Current: servo register raw → milliamps
    static inline int _rawCurrentToMa(int r) { return (int)((long)r * ST_CURRENT_UNIT_UA / 1000); }

    uint8_t _id          = 0;
    // Internal state always stored in effective units
    int     _lastSpeed   = 1000;  // [steps/s]
    int     _lastPos     = -1;    // [encoder ticks, 0–4095]
    bool    _torqueOn    = false;
    bool    _initialized = false;

    static HardwareSerial* scSerial;
};

} // namespace enableit
