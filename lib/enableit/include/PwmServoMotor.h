//
// PwmServoMotor: traditional RC/PWM servo (50 Hz, 544–2400 µs pulse)
// Wraps ServoESP32 (roboticsbrno/ServoESP32) as an enableit::Motor
//
// Position range:
//   EFFECTIVE   : 0–180 (degrees)
//   NORMALIZED  : 0–1000
//
// speed / acc parameters are accepted but ignored (PWM servos are
// rate-limited by the servo's own mechanical response).
//
// Author: A.Navatta

#pragma once

#include <Arduino.h>
#include <Servo.h>   // ServoESP32 — typedef Servo = ServoTemplate<int>
#include <Motor.h>

// ── PWM servo constants ───────────────────────────────────────────────────────
#define PWM_SERVO_MIN_US     544    // minimum pulse width [µs]  (full CW)
#define PWM_SERVO_MAX_US     2400   // maximum pulse width [µs]  (full CCW)
#define PWM_SERVO_FREQ_HZ    50     // standard RC servo frequency
#define PWM_SERVO_MIN_DEG    0      // minimum angle [°]
#define PWM_SERVO_MAX_DEG    180    // maximum angle [°]

// ─────────────────────────────────────────────────────────────────────────────

namespace enableit {

class PwmServoMotor : public Motor {
public:
    PwmServoMotor() = default;

    // Hardware setup — call once before begin()
    // pin    : any ESP32 GPIO capable of PWM output
    // channel: explicit LEDC channel (0-7). Use CHANNEL_NOT_ATTACHED (-1) for
    //          auto-assignment, but beware conflicts with M5GFX backlight (ch 0).
    void init(int pin,
              int channel = 1,
              int minDeg = PWM_SERVO_MIN_DEG,
              int maxDeg = PWM_SERVO_MAX_DEG,
              int minUs  = PWM_SERVO_MIN_US,
              int maxUs  = PWM_SERVO_MAX_US);

    // ── Motor abstract interface ──────────────────────────────────────────────
    bool begin()   override;
    void end()     override;
    void stop()    override;   // writes centre position (90°)
    void brake()   override;   // same as stop() for PWM servo
    void enable()  override;   // re-attach pin
    void disable() override;   // detach pin (servo goes limp)

    // forward: moves toward maxDeg
    // reverse: moves toward minDeg
    bool forward(int speed = -1, int acc = 0) override;
    bool reverse(int speed = -1, int acc = 0) override;

    bool setSpeed(int /*value*/) override { return false; } // not applicable
    bool setPosition(int value, int speed = -1, int acc = 0) override;

    int  getPosition() override;
    int  getSpeed()    override { return 0; }  // no speed feedback
    int  getCurrent()  override { return -1; } // no current sense

    MotorCapabilities getCapabilities() const override;
    MotorState        getState()        const override;
    MotorTelemetry    getTelemetry()    const override;
    const char*       getType()         const override { return "PwmServoMotor"; }

private:
    // ── Value-mode helpers ───────────────────────────────────────────────────
    static inline int _remap(int x, int inMin, int inMax, int outMin, int outMax) {
        if (inMax == inMin) return outMin;
        return outMin + (int)((long)(x - inMin) * (outMax - outMin) / (inMax - inMin));
    }
    // Normalized [0,1000] ↔ degree angle [minDeg, maxDeg]
    inline int _normToDeg(int n) const { return _remap(n, 0, 1000, _minDeg, _maxDeg); }
    inline int _degToNorm(int d) const { return _remap(d, _minDeg, _maxDeg, 0, 1000); }

    // Convert user value (in current mode) to degrees (internal unit)
    inline int _toDeg(int v) const {
        return (_valueMode == MotorValueMode::NORMALIZED) ? _normToDeg(v) : v;
    }
    // Convert degrees to output value (in current mode)
    inline int _fromDeg(int d) const {
        return (_valueMode == MotorValueMode::NORMALIZED) ? _degToNorm(d) : d;
    }

    ::Servo _servo;       // ServoESP32 instance (:: to avoid namespace clash)
    int  _pin     = -1;
    int  _channel = 1;   // LEDC channel; default 1 to avoid M5GFX backlight (ch 0)
    int  _minDeg  = PWM_SERVO_MIN_DEG;
    int  _maxDeg  = PWM_SERVO_MAX_DEG;
    int  _minUs   = PWM_SERVO_MIN_US;
    int  _maxUs   = PWM_SERVO_MAX_US;
    int  _posDeg  = -1;   // last commanded position [degrees]
    bool _attached = false;
    bool _enabled  = false;
};

} // namespace enableit
