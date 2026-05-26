//
// Motor: abstract base class for actuator drivers
//
// Supports:
//   - Simple bidirectional motors and linear actuators
//   - Smart servo motors (position + telemetry)
//   - PWM servos
//   - Any future actuator implementing this interface
//
// Author: A.Navatta

#pragma once

#include <Arduino.h>

namespace enableit {

// ── Value mode ────────────────────────────────────────────────────────────────
// EFFECTIVE  : native hardware values / real hardware ranges
//              PQ12 position = ADC calibrated range
//              DCMotor speed = PWM native range [0..1023]
//              SmartServo position = protocol ticks [0..4095]
//
// NORMALIZED : cross-device logical ranges (suitable for CLI / BLE / Web UI)
//              speed        -1000 .. 1000  (negative = reverse)
//              position         0 .. 1000
//              acceleration     0 .. 1000
enum class MotorValueMode {
    EFFECTIVE  = 0,
    NORMALIZED = 1,
};

// ── Capability flags ──────────────────────────────────────────────────────────
enum class MotorCapability : uint32_t {
    NONE              = 0,
    SPEED_CONTROL     = (1u <<  0),  // setSpeed() / forward(speed) / reverse(speed)
    DIRECTION_CONTROL = (1u <<  1),  // forward() / reverse()
    POSITION_SET      = (1u <<  2),  // setPosition() / move()
    POSITION_READ     = (1u <<  3),  // getPosition() / telemetry.position valid
    CURRENT_SENSE     = (1u <<  4),  // getCurrent() / telemetry.current_mA valid
    VOLTAGE_SENSE     = (1u <<  5),  // telemetry.voltage_mV valid
    TEMPERATURE_SENSE = (1u <<  6),  // telemetry.temperature_c valid
    ENABLE_DISABLE    = (1u <<  7),  // enable() / disable()
    BRAKE             = (1u <<  8),  // brake() actively holds
    FAULT_STATUS      = (1u <<  9),  // telemetry.fault / faultCode valid
    HOMING            = (1u << 10),  // home() supported
    LIMIT_SWITCHES    = (1u << 11),  // hardware limit switches present
    VELOCITY_READ     = (1u << 12),  // telemetry.velocity valid
    ACCELERATION_SET  = (1u << 13),  // setAcceleration() supported
    TORQUE_CONTROL    = (1u << 14),  // torque/force control
};

// ── Value ranges ──────────────────────────────────────────────────────────────
struct MotorRange {
    int min  = 0;
    int max  = 0;
    int step = 1;
};

// ── Capability descriptor ─────────────────────────────────────────────────────
// Returned by getCapabilities(); describes flags and ranges in both modes.
// Unsupported ranges stay zeroed; check flags before using range values.
struct MotorCapabilities {
    uint32_t flags = 0;

    MotorRange speedEffective;          // e.g. [-1023..1023] for PWM
    MotorRange speedNormalized;         // always [-1000..1000]

    MotorRange positionEffective;       // e.g. [0..4095] for smart servo
    MotorRange positionNormalized;      // always [0..1000]

    MotorRange accelerationEffective;   // driver-specific units
    MotorRange accelerationNormalized;  // always [0..1000]

    MotorRange currentEffective;        // mA
};

// ── Motor operating state ─────────────────────────────────────────────────────
enum class MotorState {
    UNKNOWN     = 0,  // not yet initialized
    IDLE,             // powered, not moving
    RUNNING,          // moving at speed (no target position)
    MOVING,           // moving toward a target position
    HOLDING,          // actively holding position (servo / stepper)
    BRAKING,          // actively braking
    CALIBRATING,      // calibration in progress
    HOMING,           // homing sequence in progress
    SLEEPING,         // low-power / disabled
    FAULT,            // fault condition active
    MOTOR_OFF,        // explicitly powered off via disable()
};

// ── Telemetry snapshot ────────────────────────────────────────────────────────
struct MotorTelemetry {
    int      position       = -1;    // current position; -1 = unavailable
    int      targetPosition = -1;    // target position; -1 = none
    int      speed          =  0;    // current speed (signed: + forward, - reverse)
    int      velocity       =  0;    // measured velocity (ticks/s or rpm)
    int      current_mA     = -1;    // instantaneous current; -1 = unavailable
    int      voltage_mV     = -1;    // supply voltage; -1 = unavailable
    int      temperature_c  = -1;    // driver/motor temperature; -1 = unavailable
    bool     enabled        = true;  // false when sleeping / disabled
    bool     moving         = false; // true if actively moving
    bool     homed          = false; // true after successful home()
    bool     fault          = false;
    uint32_t faultCode      =  0;    // driver-specific fault bits
    uint32_t timestamp      =  0;    // millis() at snapshot time
};

// ── Abstract Motor interface ──────────────────────────────────────────────────
class Motor {
public:
    virtual ~Motor() = default;

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    virtual bool begin() = 0;
    virtual void end()   = 0;

    // ── Value mode ────────────────────────────────────────────────────────────
    // All motion methods use the active value mode automatically.
    // Default implementation stores the mode and returns it.
    virtual void           setValueMode(MotorValueMode mode) { _valueMode = mode; }
    virtual MotorValueMode getValueMode() const               { return _valueMode; }

    // ── Directional control ───────────────────────────────────────────────────
    // speed = -1   → use last configured speed (or full speed if never set)
    // acc   =  0   → no acceleration ramp (immediate)
    // DC motors    : forward / reverse apply to rotation direction
    // Linear act.  : forward = extend, reverse = retract
    // PWM servos   : may return false if unsupported
    virtual bool forward(int speed = -1, int acc = 0) = 0;
    virtual bool reverse(int speed = -1, int acc = 0) = 0;

    // Coast to stop (free-wheel — no active hold)
    virtual void stop()  = 0;

    // Actively hold / short-circuit windings (BRAKE capability required)
    virtual void brake() = 0;

    // ── Power state ───────────────────────────────────────────────────────────
    virtual void enable()  = 0;  // restore power / torque
    virtual void disable() = 0;  // remove power / torque (ENABLE_DISABLE required)

    // ── Generic control ───────────────────────────────────────────────────────
    // setSpeed     : absolute magnitude; direction unchanged
    virtual bool setSpeed(int value) = 0;

    // setPosition  : absolute target position
    virtual bool setPosition(int value, int speed = -1, int acc = 0) = 0;

    // setAcceleration: default returns false (override if ACCELERATION_SET)
    virtual bool setAcceleration(int value) { (void)value; return false; }

    // move: relative displacement; default uses setPosition + getPosition
    virtual bool move(int delta, int speed = -1, int acc = 0) {
        if (!hasCapability(MotorCapability::POSITION_READ) ||
            !hasCapability(MotorCapability::POSITION_SET))
            return false;
        int cur = getPosition();
        if (cur < 0) return false;
        return setPosition(cur + delta, speed, acc);
    }

    // home: execute homing sequence (HOMING capability required)
    // Default returns false
    virtual bool home() { return false; }

    // poll: periodic update — position/current protection, encoder refresh, etc.
    // Called from the application process loop. Default: no-op.
    virtual void poll() {}

    // ── Lightweight getters ───────────────────────────────────────────────────
    // These are preferred over getTelemetry() in tight loops.
    // Values are in the active value mode.
    virtual int getPosition() = 0;
    virtual int getSpeed()    = 0;
    virtual int getCurrent()  = 0;

    // ── State / metadata ──────────────────────────────────────────────────────
    virtual MotorCapabilities getCapabilities() const = 0;
    virtual MotorState        getState()        const = 0;
    virtual MotorTelemetry    getTelemetry()    const = 0;

    // Driver-specific error code; 0 = no error
    virtual uint32_t getError() const { return 0; }

    // Identity string (e.g. "PQ12Actuator", "STMotor", "DCMotor")
    virtual const char* getType() const = 0;

    // ── Capability helpers ────────────────────────────────────────────────────
    bool hasCapability(MotorCapability cap) const {
        return (getCapabilities().flags & static_cast<uint32_t>(cap)) != 0;
    }
    bool supportsSpeed()        const { return hasCapability(MotorCapability::SPEED_CONTROL);    }
    bool supportsPosition()     const { return hasCapability(MotorCapability::POSITION_SET);     }
    bool supportsCurrent()      const { return hasCapability(MotorCapability::CURRENT_SENSE);    }
    bool supportsAcceleration() const { return hasCapability(MotorCapability::ACCELERATION_SET); }

protected:
    MotorValueMode _valueMode = MotorValueMode::EFFECTIVE;
};

} // namespace enableit