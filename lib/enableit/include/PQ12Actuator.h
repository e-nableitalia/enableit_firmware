//
// PQ12Actuator: TI DRV8xxx H-bridge driver (DRV8411, DRV8833, DRV8835)
// Concrete implementation of enableit::Motor abstract interface
//
// Author: A.Navatta

#pragma once

#include <Arduino.h>
#include <Motor.h>

// ── Board-specific driver selection ──────────────────────────────────────────
#ifdef ARDUINO_M5Stack_ATOMS3

//#define DRIVER_DRV8833
//#define DRIVER_DRV8835
#define DRIVER_DRV8411

#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
// Define physical enable / fault pins for your board
#define PQ12_ENABLE_PIN   -1
#define PQ12_FAULT_PIN    -1
#endif

#else
#pragma message "PQ12Actuator: define driver type and pins for your board"
#endif

// ── PWM / speed constants ────────────────────────────────────────────────────
#define MOTOR_SPEED_BIT         (uint8_t)10
#define MOTOR_SPEED_MAX         ((1 << MOTOR_SPEED_BIT) - 1)   // 1023
#define MOTOR_PWM_FREQ          78000

// ── ADC constants (ESP32 12-bit, 3.3 V reference) ────────────────────────────
#define PQ12_ADC_BITS           12
#define PQ12_ADC_MAX            ((1 << PQ12_ADC_BITS) - 1)     // 4095
#define PQ12_ADC_VREF_MV        3300

// ── PQ12 Actuonix wiper position (ADC raw counts) ────────────────────────────
// Soft limits: motor stops before the physical endstop to prevent stall.
// Calibrate these values for your specific actuator unit.
#define PQ12_POS_SOFT_MIN       1700
#define PQ12_POS_SOFT_MAX       3950

// ── DRV8411 current sense (IPROPI pin) ───────────────────────────────────────
// IOUT_mA = (ADC_raw × VREF_mV × ISENSE_RATIO) / (ADC_MAX × RSENSE_Ω)
// Adjust DRV8411_RSENSE_OHMS to match the resistor fitted on your board.
#ifdef DRIVER_DRV8411
#  define DRV8411_ISENSE_RATIO  5000   // IOUT : IPROPI ratio (DRV8411 datasheet)
#  define DRV8411_RSENSE_OHMS   1000   // Rsense from IPROPI to GND [Ω]
#endif

// Over-current protection threshold [mA] — motor stopped above this value
#define PQ12_CURRENT_MAX_MA     1500

// Enable hardware protection features
//#define MOTOR_POSITION_PROTECTION 1
#define MOTOR_CURRENT_PROTECTION  1

// ── Decay mode (0 = slow, 1 = fast) ──────────────────────────────────────────
#define MOTOR_FASTDECAY_MODE    0

// ─────────────────────────────────────────────────────────────────────────────

namespace enableit {

class PQ12Actuator : public Motor {
public:
    PQ12Actuator();

    // Hardware setup — call once before begin()
    void init(int in1Pin, int in2Pin,
              int wiperPin   = -1,
              int isensePin  = -1,
              bool enableSpeed = false);

    // ── Motor abstract interface ──────────────────────────────────────────────
    bool begin()  override;
    void end()    override;
    void stop()   override;
    void brake()  override;
    void enable()  override;
    void disable() override;

    // speed = -1 → use last configured speed; extend = forward, retract = reverse
    bool forward(int speed = -1, int acc = 0) override;
    bool reverse(int speed = -1, int acc = 0) override;

    bool setSpeed(int value)                             override;
    bool setPosition(int value, int speed = -1, int acc = 0) override;  // not supported — always returns false

    int  getPosition() override;
    int  getSpeed()    override { return _currentSpeed; }
    int  getCurrent()  override { return _telemetry.current_mA; }

    MotorCapabilities getCapabilities() const override;
    MotorState        getState()        const override;
    MotorTelemetry    getTelemetry()    const override;
    const char*       getType()         const override { return "PQ12Actuator"; }

    // ── Additional methods ────────────────────────────────────────────────────
    void stop(const String& reason); // logs reason then calls stop()
    void sleep();   // alias for disable()
    void wakeup();  // alias for enable()
    void poll();

private:
    enum class Direction { FORWARD, REVERSE, STOPPED };

    void _applyStop();   // internal: applies stop signals to pins

    // ── Value-mode conversion helpers ─────────────────────────────────────────
    // Integer remap: maps x in [inMin, inMax] → [outMin, outMax]
    static inline int _remap(int x, int inMin, int inMax, int outMin, int outMax) {
        if (inMax == inMin) return outMin;
        return outMin + (int)((long)(x - inMin) * (outMax - outMin) / (inMax - inMin));
    }
    // Normalized speed magnitude [0, 1000] → PWM ticks [0, MOTOR_SPEED_MAX]
    static inline int _normSpeedToPwm(int n) {
        return _remap(n, 0, 1000, 0, MOTOR_SPEED_MAX);
    }
    // ADC wiper raw → normalized position [0, 1000]  (clamped to soft limits)
    static inline int _rawPosToNorm(int raw) {
        return _remap(raw, PQ12_POS_SOFT_MIN, PQ12_POS_SOFT_MAX, 0, 1000);
    }
    // Normalized position [0, 1000] → ADC raw
    static inline int _normPosToRaw(int norm) {
        return _remap(norm, 0, 1000, PQ12_POS_SOFT_MIN, PQ12_POS_SOFT_MAX);
    }
#ifdef DRIVER_DRV8411
    // DRV8411 IPROPI pin ADC raw → motor output current [mA]
    static inline int _rawCurrentToMa(int raw) {
        return (int)((long)raw * PQ12_ADC_VREF_MV * DRV8411_ISENSE_RATIO
                     / ((long)PQ12_ADC_MAX * DRV8411_RSENSE_OHMS));
    }
#endif

    int  _in1Pin        = -1;
    int  _in2Pin        = -1;
    int  _wiperPin      = -1;
    int  _isensePin     = -1;
    bool _speedControl  = false;
    bool _initialized   = false;
    bool _enabled       = true;
    bool _sleeping      = false;
    bool _braking       = false;
    int  _currentSpeed  = 0;
    int  _currentMaxMa  = 0;    // peak measured current [mA]
    mutable int _rawPosition = -1;  // last ADC wiper reading (always raw counts)

    Direction _direction = Direction::STOPPED;

    mutable MotorTelemetry _telemetry;
};

} // namespace enableit
