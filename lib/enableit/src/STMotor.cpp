// STMotor: Waveshare ST series servo (ST3215, SMS_STS family)
// Implements enableit::Motor abstract interface
// Author: A.Navatta

#include "Console.h"
#include "STMotor.h"

using namespace enableit;

HardwareSerial* STMotor::scSerial = nullptr;

// ── Constructor / init ─────────────────────────────────────────────────────────────
STMotor::STMotor()
    : _id(0), _lastPos(-1), _lastSpeed(1000), _torqueOn(false), _initialized(false), SMS_STS()
{
}

void STMotor::static_init(int sRX, int sTX)
{
    scSerial = &Serial2;
    log_d("STMotor: initializing Serial2 RX=%d TX=%d", sRX, sTX);
    scSerial->begin(1000000, SERIAL_8N1, sRX, sTX);
}

void STMotor::init(uint8_t servoId)
{
    _id    = servoId;
    pSerial = scSerial;
    log_d("STMotor: init id=%d", _id);
}

// ── Motor lifecycle ──────────────────────────────────────────────────────────────
bool STMotor::begin()
{
    if (scSerial == nullptr) {
        log_e("STMotor::begin — static_init() not called");
        return false;
    }
    pSerial = scSerial;
    EnableTorque(_id, 1);
    _torqueOn    = true;
    _initialized = true;
    log_d("STMotor::begin id=%d", _id);
    return true;
}

void STMotor::end()
{
    EnableTorque(_id, 0);
    _torqueOn    = false;
    _initialized = false;
    log_d("STMotor::end id=%d", _id);
}

// ── Stop / brake ──────────────────────────────────────────────────────────────
void STMotor::stop()
{
    // Coast: disable torque, servo is free to move
    EnableTorque(_id, 0);
    _torqueOn = false;
    log_d("STMotor::stop id=%d", _id);
}

void STMotor::brake()
{
    // Servo "brake": re-enable torque at current position so it holds
    int pos = ReadPos(_id);
    if (pos >= 0) {
        WritePosEx(_id, pos, 0, 0);
        EnableTorque(_id, 1);
        _torqueOn = true;
        log_d("STMotor::brake id=%d hold at pos=%d", _id, pos);
    } else {
        // Can't read position — fall back to torque disable
        EnableTorque(_id, 0);
        _torqueOn = false;
        log_d("STMotor::brake id=%d — pos read failed, torque off", _id);
    }
}

void STMotor::enable()
{
    EnableTorque(_id, 1);
    _torqueOn = true;
    log_d("STMotor::enable id=%d", _id);
}

void STMotor::disable()
{
    EnableTorque(_id, 0);
    _torqueOn = false;
    log_d("STMotor::disable id=%d", _id);
}

// ── Directional control ───────────────────────────────────────────────────────────
bool STMotor::forward(int speed, int acc)
{
    // Inputs are in the current value mode; convert to effective units internally
    int spd = (speed < 0) ? _lastSpeed
              : (_valueMode == MotorValueMode::NORMALIZED) ? _normSpeedToEff(speed) : speed;
    int aEff = (_valueMode == MotorValueMode::NORMALIZED) ? _normAccToEff(acc) : acc;
    WritePosEx(_id, ST_POS_MAX, spd, (uint8_t)aEff);
    _lastPos   = ST_POS_MAX;
    _lastSpeed = spd;
    log_d("STMotor::forward id=%d spd=%d acc=%d", _id, spd, aEff);
    return true;
}

bool STMotor::reverse(int speed, int acc)
{
    int spd = (speed < 0) ? _lastSpeed
              : (_valueMode == MotorValueMode::NORMALIZED) ? _normSpeedToEff(speed) : speed;
    int aEff = (_valueMode == MotorValueMode::NORMALIZED) ? _normAccToEff(acc) : acc;
    WritePosEx(_id, ST_POS_MIN, spd, (uint8_t)aEff);
    _lastPos   = ST_POS_MIN;
    _lastSpeed = spd;
    log_d("STMotor::reverse id=%d spd=%d acc=%d", _id, spd, aEff);
    return true;
}

// ── Speed / position ────────────────────────────────────────────────────────────
bool STMotor::setSpeed(int value)
{
    // Store always in effective units
    int spd = (_valueMode == MotorValueMode::NORMALIZED) ? _normSpeedToEff(value) : value;
    _lastSpeed = spd;
    if (_lastPos >= 0)
        WritePosEx(_id, _lastPos, _lastSpeed, 0);
    log_d("STMotor::setSpeed id=%d spd=%d", _id, _lastSpeed);
    return true;
}

bool STMotor::setPosition(int value, int speed, int acc)
{
    return moveTo(value, speed, acc);
}

bool STMotor::move(int delta, int speed, int acc)
{
    int cur = ReadPos(_id);
    if (cur < 0) {
        log_d("STMotor::move id=%d — cannot read position", _id);
        return false;
    }
    // delta is in current value mode; convert to effective ticks before adding
    int effDelta = (_valueMode == MotorValueMode::NORMALIZED)
                   ? (int)((long)delta * (ST_POS_MAX - ST_POS_MIN) / 1000)
                   : delta;
    return moveTo(cur + effDelta, speed, acc);
}

bool STMotor::moveTo(int position, int speed, int acc)
{
    int posEff = (_valueMode == MotorValueMode::NORMALIZED) ? _normPosToEff(position) : position;
    int spd    = (speed < 0) ? _lastSpeed
                 : (_valueMode == MotorValueMode::NORMALIZED) ? _normSpeedToEff(speed) : speed;
    int aEff   = (_valueMode == MotorValueMode::NORMALIZED) ? _normAccToEff(acc) : acc;
    WritePosEx(_id, posEff, spd, (uint8_t)aEff);
    _lastPos   = posEff;
    _lastSpeed = spd;
    log_d("STMotor::moveTo id=%d pos=%d spd=%d acc=%d", _id, posEff, spd, aEff);
    return true;
}

// ── Introspection ────────────────────────────────────────────────────────────────
MotorCapabilities STMotor::getCapabilities() const
{
    MotorCapabilities caps;
    caps.flags =
        static_cast<uint32_t>(MotorCapability::DIRECTION_CONTROL)
      | static_cast<uint32_t>(MotorCapability::SPEED_CONTROL)
      | static_cast<uint32_t>(MotorCapability::POSITION_SET)
      | static_cast<uint32_t>(MotorCapability::POSITION_READ)
      | static_cast<uint32_t>(MotorCapability::CURRENT_SENSE)
      | static_cast<uint32_t>(MotorCapability::TEMPERATURE_SENSE)
      | static_cast<uint32_t>(MotorCapability::VOLTAGE_SENSE)
      | static_cast<uint32_t>(MotorCapability::ENABLE_DISABLE)
      | static_cast<uint32_t>(MotorCapability::BRAKE)
      | static_cast<uint32_t>(MotorCapability::ACCELERATION_SET)
      | static_cast<uint32_t>(MotorCapability::FAULT_STATUS);

    caps.positionEffective        = { ST_POS_MIN,  ST_POS_MAX,  1 };
    caps.positionNormalized       = { 0,           1000,        1 };
    caps.speedEffective           = { 0,           ST_SPEED_MAX, 1 };
    caps.speedNormalized          = { 0,           1000,         1 };
    caps.accelerationEffective    = { 0,           ST_ACC_MAX,  1 };
    caps.accelerationNormalized   = { 0,           1000,        1 };
    caps.currentEffective         = { 0,           _rawCurrentToMa(0x7FF), 1 }; // 11-bit current reg
    return caps;
}

MotorState STMotor::getState() const
{
    if (!_initialized)          return MotorState::UNKNOWN;
    if (Error != 0)             return MotorState::FAULT;
    if (!_torqueOn)             return MotorState::SLEEPING;
    if (_lastPos >= 0) {
        int cur = const_cast<STMotor*>(this)->ReadPos(_id);
        if (cur >= 0 && abs(cur - _lastPos) > 10)
            return MotorState::MOVING;
        return MotorState::HOLDING;
    }
    return MotorState::IDLE;
}

MotorTelemetry STMotor::getTelemetry() const
{
    MotorTelemetry t;
    t.timestamp      = millis();
    t.enabled        = _torqueOn;
    t.fault          = (Error != 0);
    t.faultCode      = static_cast<uint32_t>(Error);

    // FeedBack reads all servo registers in one burst; ReadXxx(-1) returns cached values
    int fb = const_cast<SMS_STS*>(static_cast<const SMS_STS*>(this))->FeedBack(_id);
    if (fb != -1) {
        SMS_STS* sms = const_cast<SMS_STS*>(static_cast<const SMS_STS*>(this));
        int rawPos  = sms->ReadPos(-1);
        int rawSpd  = sms->ReadSpeed(-1);
        int rawCur  = sms->ReadCurrent(-1);
        t.temperature_c = sms->ReadTemper(-1);
        t.voltage_mV    = sms->ReadVoltage(-1) * 100;  // 0.1 V units → mV
        t.current_mA    = _rawCurrentToMa(rawCur);
        t.moving        = (abs(rawSpd) > 0);

        // Apply value-mode conversion for position and speed
        if (_valueMode == MotorValueMode::NORMALIZED) {
            t.position       = _effPosToNorm(rawPos);
            t.targetPosition = _effPosToNorm(_lastPos);
            t.velocity       = _effSpeedToNorm(abs(rawSpd)) * (rawSpd >= 0 ? 1 : -1);
            t.speed          = _effSpeedToNorm(_lastSpeed);
        } else {
            t.position       = rawPos;
            t.targetPosition = _lastPos;
            t.velocity       = rawSpd;
            t.speed          = _lastSpeed;
        }
    } else {
        // No FeedBack — use cached state only
        t.targetPosition = (_valueMode == MotorValueMode::NORMALIZED)
                           ? _effPosToNorm(_lastPos) : _lastPos;
        t.speed          = (_valueMode == MotorValueMode::NORMALIZED)
                           ? _effSpeedToNorm(_lastSpeed) : _lastSpeed;
    }
    return t;
}

uint32_t STMotor::getError() const
{
    return static_cast<uint32_t>(Error);
}

// ── Poll / helpers ───────────────────────────────────────────────────────────────
void STMotor::poll()
{
    _lastPos = ReadPos(_id);
}

int STMotor::getPosition()
{
    int raw = ReadPos(_id);
    if (raw < 0) return raw;
    return (_valueMode == MotorValueMode::NORMALIZED) ? _effPosToNorm(raw) : raw;
}

int STMotor::getSpeed()
{
    return (_valueMode == MotorValueMode::NORMALIZED) ? _effSpeedToNorm(_lastSpeed) : _lastSpeed;
}

int STMotor::getCurrent()
{
    return _rawCurrentToMa(ReadCurrent(_id));
}

