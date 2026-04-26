//
// PQ12Actuator: TI DRV8xxx H-bridge driver implementation
//
// Author: A.Navatta

#include <PQ12Actuator.h>
#include <Console.h>
#include <ESP32PWM.h>

using namespace enableit;

// ── Constructor ───────────────────────────────────────────────────────────────

PQ12Actuator::PQ12Actuator()
{
}

// ── Hardware setup ────────────────────────────────────────────────────────────

void PQ12Actuator::init(int in1Pin, int in2Pin, int wiperPin, int isensePin, bool enableSpeed)
{
    _in1Pin       = in1Pin;
    _in2Pin       = in2Pin;
    _wiperPin     = wiperPin;
    _isensePin    = isensePin;
    _speedControl = enableSpeed;

    log_d("PQ12Actuator init in1=%d in2=%d wiper=%d isense=%d speedCtrl=%d",
          in1Pin, in2Pin, wiperPin, isensePin, (int)enableSpeed);

    begin();
}

// ── Motor interface ───────────────────────────────────────────────────────────

bool PQ12Actuator::begin()
{
    if (_in1Pin == -1 || _in2Pin == -1) {
        log_e("PQ12Actuator::begin — pins not configured");
        return false;
    }

    pinMode(_in1Pin, OUTPUT);
    pinMode(_in2Pin, OUTPUT);

#if defined(DRIVER_DRV8411) || defined(DRIVER_DRV8833)
    if (_speedControl) {
        analogWriteResolution(MOTOR_SPEED_BIT);
        analogWriteFrequency(MOTOR_PWM_FREQ);
    }
#endif

#ifdef PQ12_FAULT_PIN
    if (PQ12_FAULT_PIN != -1)
        pinMode(PQ12_FAULT_PIN, INPUT_PULLUP);
#endif

#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
    if (PQ12_ENABLE_PIN != -1) {
        pinMode(PQ12_ENABLE_PIN, OUTPUT);
        digitalWrite(PQ12_ENABLE_PIN, HIGH);
    }
#endif

    if (_isensePin != -1) {
        pinMode(_isensePin, INPUT);
        log_d("PQ12Actuator: current sense on pin %d", _isensePin);
    }
    if (_wiperPin != -1) {
        pinMode(_wiperPin, INPUT);
        _rawPosition = analogRead(_wiperPin);
        log_d("PQ12Actuator: wiper on pin %d, initial raw=%d", _wiperPin, _rawPosition);
    }

    _initialized  = true;
    _enabled      = true;
    _sleeping     = false;
    _currentSpeed = 0;
    _currentMaxMa = 0;
    _rawPosition  = -1;
    _telemetry    = MotorTelemetry{};

    _applyStop();
    poll();

    log_d("PQ12Actuator initialized");
    return true;
}

void PQ12Actuator::end()
{
    sleep();
    _initialized = false;
}

// ── Stop ──────────────────────────────────────────────────────────────────────

void PQ12Actuator::_applyStop()
{
#ifdef DRIVER_DRV8835
    digitalWrite(_in1Pin, HIGH);
    digitalWrite(_in2Pin, HIGH);
#else  // DRV8411, DRV8833 — coast / brake
    if (_speedControl) {
        analogWrite(_in1Pin, MOTOR_SPEED_MAX);
        analogWrite(_in2Pin, MOTOR_SPEED_MAX);
    } else {
        digitalWrite(_in1Pin, HIGH);
        digitalWrite(_in2Pin, HIGH);
    }
#endif
    _direction    = Direction::STOPPED;
    _currentSpeed = 0;
    _braking      = false;
}

void PQ12Actuator::stop()
{
    _applyStop();
    log_d("PQ12Actuator stop");
}

void PQ12Actuator::stop(const String& reason)
{
    log_d("PQ12Actuator stop: %s", reason.c_str());
    stop();
}

// ── Speed / position ──────────────────────────────────────────────────────────

bool PQ12Actuator::setSpeed(int value)
{
    int pwm = abs(value);
    if (pwm > MOTOR_SPEED_MAX) pwm = MOTOR_SPEED_MAX;

    if (value == 0) {
        stop();
        return true;
    }

    if (value > 0)
        return forward(pwm);
    else
        return reverse(pwm);
}

bool PQ12Actuator::setPosition(int /*value*/, int /*speed*/, int /*acc*/)
{
    // Analog feedback is read-only — no closed-loop position control
    return false;
}

// ── Direction primitives ──────────────────────────────────────────────────────
// speed = -1 → use last abs speed (or MOTOR_SPEED_MAX if never set)

bool PQ12Actuator::forward(int speed, int /*acc*/)
{
    int pwmSpeed;
    if (speed < 0)
        pwmSpeed = abs(_currentSpeed);
    else if (_valueMode == MotorValueMode::NORMALIZED)
        pwmSpeed = _normSpeedToPwm(speed);
    else
        pwmSpeed = speed;
    if (pwmSpeed == 0 || pwmSpeed > MOTOR_SPEED_MAX) pwmSpeed = MOTOR_SPEED_MAX;

#ifdef DRIVER_DRV8835
    digitalWrite(_in1Pin, HIGH);
    digitalWrite(_in2Pin, LOW);
    log_d("PQ12Actuator forward");
#else
    if (_speedControl) {
#ifdef MOTOR_FASTDECAY_MODE
        analogWrite(_in1Pin, pwmSpeed);
        analogWrite(_in2Pin, 0);
        log_d("PQ12Actuator forward fast speed=%d", pwmSpeed);
#else
        analogWrite(_in1Pin, MOTOR_SPEED_MAX);
        analogWrite(_in2Pin, pwmSpeed);
        log_d("PQ12Actuator forward slow speed=%d", pwmSpeed);
#endif
    } else {
        digitalWrite(_in1Pin, HIGH);
        digitalWrite(_in2Pin, LOW);
        log_d("PQ12Actuator forward");
    }
#endif
    _direction    = Direction::FORWARD;
    _currentSpeed = pwmSpeed;
    return true;
}

bool PQ12Actuator::reverse(int speed, int /*acc*/)
{
    int pwmSpeed;
    if (speed < 0)
        pwmSpeed = abs(_currentSpeed);
    else if (_valueMode == MotorValueMode::NORMALIZED)
        pwmSpeed = _normSpeedToPwm(speed);
    else
        pwmSpeed = speed;
    if (pwmSpeed == 0 || pwmSpeed > MOTOR_SPEED_MAX) pwmSpeed = MOTOR_SPEED_MAX;

#ifdef DRIVER_DRV8835
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, HIGH);
    log_d("PQ12Actuator reverse");
#else
    if (_speedControl) {
#ifdef MOTOR_FASTDECAY_MODE
        analogWrite(_in1Pin, 0);
        analogWrite(_in2Pin, pwmSpeed);
        log_d("PQ12Actuator reverse fast speed=%d", pwmSpeed);
#else
        analogWrite(_in1Pin, pwmSpeed);
        analogWrite(_in2Pin, MOTOR_SPEED_MAX);
        log_d("PQ12Actuator reverse slow speed=%d", pwmSpeed);
#endif
    } else {
        digitalWrite(_in1Pin, LOW);
        digitalWrite(_in2Pin, HIGH);
        log_d("PQ12Actuator reverse");
    }
#endif
    _direction    = Direction::REVERSE;
    _currentSpeed = -pwmSpeed;
    return true;
}

void PQ12Actuator::brake()
{
    // DRV8411/DRV8833/DRV8835: IN1=HIGH, IN2=HIGH → active brake (short-circuit)
    digitalWrite(_in1Pin, HIGH);
    digitalWrite(_in2Pin, HIGH);
    _direction    = Direction::STOPPED;
    _currentSpeed = 0;
    _braking      = true;
    log_d("PQ12Actuator brake");
}

// ── Enable / disable ──────────────────────────────────────────────────────────

void PQ12Actuator::enable()
{
    wakeup();
}

void PQ12Actuator::disable()
{
    sleep();
}

void PQ12Actuator::sleep()
{
#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
    if (PQ12_ENABLE_PIN != -1) {
        digitalWrite(PQ12_ENABLE_PIN, LOW);
    }
#else
    if (_in1Pin != -1) digitalWrite(_in1Pin, LOW);
    if (_in2Pin != -1) digitalWrite(_in2Pin, LOW);
#endif
    _sleeping = true;
    _enabled  = false;
    log_d("PQ12Actuator sleep");
}

void PQ12Actuator::wakeup()
{
#if defined(DRIVER_DRV8835) || defined(DRIVER_DRV8833)
    if (PQ12_ENABLE_PIN != -1) {
        digitalWrite(PQ12_ENABLE_PIN, HIGH);
    }
#endif
    _sleeping = false;
    _enabled  = true;
    log_d("PQ12Actuator wakeup");
}

// ── Poll (called from process loop) ──────────────────────────────────────────

void PQ12Actuator::poll()
{
#ifdef MOTOR_POSITION_PROTECTION
    if (_wiperPin != -1) {
        _rawPosition = analogRead(_wiperPin);
        if (_rawPosition > PQ12_POS_SOFT_MAX && _direction == Direction::FORWARD) {
            stop("poll: forward soft limit");
            return;
        } else if (_rawPosition < PQ12_POS_SOFT_MIN && _direction == Direction::REVERSE) {
            stop("poll: reverse soft limit");
            return;
        }
    }
#endif
    if (_isensePin != -1) {
        int raw = analogRead(_isensePin);
#ifdef DRIVER_DRV8411
        int mA = _rawCurrentToMa(raw);
#else
        int mA = raw;   // no IPROPI conversion for this driver variant
#endif
        _telemetry.current_mA = mA;
        if (mA > _currentMaxMa) {
            _currentMaxMa = mA;
            log_d("PQ12Actuator current=%d mA (peak=%d mA)", mA, _currentMaxMa);
        }
#ifdef MOTOR_CURRENT_PROTECTION
        if (mA > PQ12_CURRENT_MAX_MA) {
            stop("poll: current limit");
            return;
        }
#endif
    }
}

// ── Telemetry accessors ───────────────────────────────────────────────────────

int PQ12Actuator::getPosition()
{
    if (_wiperPin == -1) return -1;
    _rawPosition = analogRead(_wiperPin);
    return (_valueMode == MotorValueMode::NORMALIZED)
           ? _rawPosToNorm(_rawPosition)
           : _rawPosition;
}

MotorCapabilities PQ12Actuator::getCapabilities() const
{
    MotorCapabilities caps;
    caps.flags = static_cast<uint32_t>(MotorCapability::SPEED_CONTROL)
               | static_cast<uint32_t>(MotorCapability::DIRECTION_CONTROL)
               | static_cast<uint32_t>(MotorCapability::ENABLE_DISABLE)
               | static_cast<uint32_t>(MotorCapability::BRAKE);

    if (_wiperPin != -1) {
        caps.flags |= static_cast<uint32_t>(MotorCapability::POSITION_READ);
        caps.positionEffective  = { PQ12_POS_SOFT_MIN, PQ12_POS_SOFT_MAX, 1 };
        caps.positionNormalized = { 0, 1000, 1 };
    }

    if (_isensePin != -1) {
        caps.flags |= static_cast<uint32_t>(MotorCapability::CURRENT_SENSE);
#ifdef DRIVER_DRV8411
        caps.currentEffective = { 0, _rawCurrentToMa(PQ12_ADC_MAX), 1 };
#endif
    }

#if defined(PQ12_FAULT_PIN) && PQ12_FAULT_PIN != -1
    caps.flags |= static_cast<uint32_t>(MotorCapability::FAULT_STATUS);
#endif

    caps.speedEffective  = { -MOTOR_SPEED_MAX, MOTOR_SPEED_MAX, 1 };
    caps.speedNormalized = { -1000, 1000, 1 };

    return caps;
}

MotorState PQ12Actuator::getState() const
{
    if (_telemetry.fault)   return MotorState::FAULT;
    if (_sleeping)          return MotorState::SLEEPING;
    if (_braking)           return MotorState::BRAKING;
    if (_currentSpeed == 0) return MotorState::IDLE;
    return MotorState::RUNNING;
}

MotorTelemetry PQ12Actuator::getTelemetry() const
{
    // Refresh raw readings into internal state
    if (_wiperPin != -1)
        _rawPosition = analogRead(_wiperPin);
    if (_isensePin != -1) {
        int raw = analogRead(_isensePin);
#ifdef DRIVER_DRV8411
        _telemetry.current_mA = _rawCurrentToMa(raw);
#else
        _telemetry.current_mA = raw;
#endif
    }

    _telemetry.timestamp     = millis();
    _telemetry.enabled       = _enabled;
    _telemetry.voltage_mV    = -1;
    _telemetry.temperature_c = -1;

    // Build output copy with value-mode conversion applied
    MotorTelemetry out = _telemetry;
    if (_rawPosition >= 0)
        out.position = (_valueMode == MotorValueMode::NORMALIZED)
                       ? _rawPosToNorm(_rawPosition)
                       : _rawPosition;
    out.speed = (_valueMode == MotorValueMode::NORMALIZED && _currentSpeed != 0)
                ? _remap(abs(_currentSpeed), 0, MOTOR_SPEED_MAX, 0, 1000)
                  * (_currentSpeed >= 0 ? 1 : -1)
                : _currentSpeed;
    return out;
}
