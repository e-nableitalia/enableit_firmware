//
// PwmServoMotor: traditional RC/PWM servo implementation
//
// Author: A.Navatta

#include "PwmServoMotor.h"
#include "Console.h"

using namespace enableit;

// ── Setup ─────────────────────────────────────────────────────────────────────

void PwmServoMotor::init(int pin, int channel, int minDeg, int maxDeg, int minUs, int maxUs)
{
    _pin     = pin;
    _channel = channel;
    _minDeg  = minDeg;
    _maxDeg  = maxDeg;
    _minUs   = minUs;
    _maxUs   = maxUs;
    log_d("PwmServoMotor::init pin=%d ch=%d range=[%d,%d] pulse=[%d,%d]µs",
          pin, channel, minDeg, maxDeg, minUs, maxUs);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

bool PwmServoMotor::begin()
{
    if (_pin < 0) {
        log_e("PwmServoMotor::begin — pin not configured");
        return false;
    }
    // Use explicit LEDC channel to avoid conflicts (M5GFX backlight uses ch 0).
    // The 7-arg form: attach(pin, channel, minAngle, maxAngle, minUs, maxUs, freq)
    _servo.attach(_pin, _channel, _minDeg, _maxDeg, _minUs, _maxUs, PWM_SERVO_FREQ_HZ);
    _attached = _servo.attached();
    log_d("PwmServoMotor::begin pin=%d ch=%d attached=%d",
          _pin, _channel, (int)_attached);
    if (!_attached || !_servo.attached()) {
        log_e("PwmServoMotor::begin — attach failed on pin %d", _pin);
        return false;
    }
    _enabled = true;
    // Start at centre
    int centre = (_minDeg + _maxDeg) / 2;
    _servo.write(centre);
    _posDeg = centre;
    log_d("PwmServoMotor::begin pin=%d, centre=%d°, pulse=[%d,%d]µs",
          _pin, centre, _minUs, _maxUs);
    return true;
}

void PwmServoMotor::end()
{
    _servo.detach();
    _attached = false;
    _enabled  = false;
    _posDeg   = -1;
    log_d("PwmServoMotor::end");
}

// ── Stop / brake ──────────────────────────────────────────────────────────────

void PwmServoMotor::stop()
{
    if (!_attached) return;
    int centre = (_minDeg + _maxDeg) / 2;
    _servo.write(centre);
    _posDeg = centre;
    log_d("PwmServoMotor::stop (centre=%d°)", centre);
}

void PwmServoMotor::brake()
{
    stop();  // For a PWM servo "brake" == hold at centre
}

// ── Enable / disable ──────────────────────────────────────────────────────────

void PwmServoMotor::enable()
{
    if (_attached) { _enabled = true; return; }
    _servo.attach(_pin, _channel, _minDeg, _maxDeg, _minUs, _maxUs, PWM_SERVO_FREQ_HZ);
    _attached = _servo.attached();
    _enabled  = _attached;
    if (_posDeg >= 0) _servo.write(_posDeg);
    log_d("PwmServoMotor::enable attached=%d", (int)_attached);
}

void PwmServoMotor::disable()
{
    _servo.detach();
    _attached = false;
    _enabled  = false;
    log_d("PwmServoMotor::disable");
}

// ── Directional control ───────────────────────────────────────────────────────

bool PwmServoMotor::forward(int /*speed*/, int /*acc*/)
{
    return setPosition(_fromDeg(_maxDeg));
}

bool PwmServoMotor::reverse(int /*speed*/, int /*acc*/)
{
    return setPosition(_fromDeg(_minDeg));
}

// ── setPosition ───────────────────────────────────────────────────────────────

bool PwmServoMotor::setPosition(int value, int /*speed*/, int /*acc*/)
{
    if (!_attached) return false;
    int deg = _toDeg(value);
    deg = constrain(deg, _minDeg, _maxDeg);
    _servo.write(deg);
    _posDeg = deg;
    log_d("PwmServoMotor::setPosition value=%d → %d°", value, deg);
    return true;
}

// ── Getters ───────────────────────────────────────────────────────────────────

int PwmServoMotor::getPosition()
{
    if (_posDeg < 0) return -1;
    return _fromDeg(_posDeg);
}

// ── Capabilities / state / telemetry ─────────────────────────────────────────

MotorCapabilities PwmServoMotor::getCapabilities() const
{
    MotorCapabilities caps;
    caps.flags = static_cast<uint32_t>(MotorCapability::DIRECTION_CONTROL)
               | static_cast<uint32_t>(MotorCapability::POSITION_SET)
               | static_cast<uint32_t>(MotorCapability::POSITION_READ)
               | static_cast<uint32_t>(MotorCapability::ENABLE_DISABLE);
    caps.positionEffective  = { _minDeg, _maxDeg, 1 };
    caps.positionNormalized = { 0, 1000, 1 };
    return caps;
}

MotorState PwmServoMotor::getState() const
{
    if (!_enabled)  return MotorState::MOTOR_OFF;
    if (!_attached) return MotorState::UNKNOWN;
    if (_posDeg < 0) return MotorState::IDLE;
    return MotorState::HOLDING;
}

MotorTelemetry PwmServoMotor::getTelemetry() const
{
    MotorTelemetry t;
    t.timestamp      = millis();
    t.enabled        = _enabled;
    t.position       = (_posDeg >= 0) ? _fromDeg(_posDeg) : -1;
    t.targetPosition = t.position;
    t.speed          = 0;
    t.velocity       = 0;
    t.current_mA     = -1;
    t.voltage_mV     = -1;
    t.temperature_c  = -1;
    t.moving         = false;
    t.fault          = false;
    return t;
}
