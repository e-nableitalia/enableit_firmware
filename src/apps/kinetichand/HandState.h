// HandState.h
// Class abstracting hand states
// Author: A. Navatta

#ifndef HAND_STATE_H

#define HAND_STATE_H

#include <Console.h>
#include <DeltaTime.h>

#ifdef ARDUINO_M5Stack_ATOMS3
#define PRESSURE_PIN    G5
#define SERVO_PIN       G8
#else
#define PRESSURE_PIN    GPIO_NUM_8
#define SERVO_PIN       GPIO_NUM_9
#pragma warning "STUB Pressure PIN defined"
#endif

#define PRESSURE_MAX    4096
#define PRESSURE_MIN      10  // min pressure value to trigger hand closure/opening

#define SERVO_MAXANGLE    270
#define MODE_PROGRESSIVE  1   // firmware mode = hold
#define MODE_HOLDTIME     2   // 

#define INTERVAL_OPEN     0   // hand open interval id, used to calculate hold time since first muscle contraction -> activate hand/servo closure
#define INTERVAL_CLOSE    1   // hand close interval id, used to calculate hold time since first muscle contraction -> trigger hand open operation
#define HOLDTIME_INTERVAL_uS   (250 * 1000)  // 250 ms, hold time


// for servo 270 degrees maximum lever 6-7mm
#if (SERVO_MAXANGLE == 270)
#define SERVO_MAX 	254
#define SERVO_MIN	  1
#endif // SERVO 270°

// servo 180 degrees maximum lever 11-12mm
#if (SERVO_MAXANGLE == 180)
#define SERVO_MAX 	150
#define SERVO_MIN	  60
#endif // SERVO 180°

void handInit();
void updateServoProgressive();

class HandState {
public:
  virtual const char *getName() = 0;
  virtual HandState *update(int pressure) = 0;
  virtual HandState *enter() {
      return this;
  }
protected:
  DeltaTime m_interval;
};

class HandStateHoldOpening : public HandState {
  public:
    static HandState& instance();
    const char *getName() { return "HOLD_OPENING"; }
    virtual HandState *update(int pressure);
    virtual HandState *enter();
};

class HandStateIdleOpen : public HandState {
  public:
    static HandState& instance();
    const char *getName() { return "IDLE_OPEN"; }
    virtual HandState *update(int pressure);
};

class HandStateOpening : public HandState {
  public:
    static HandState& instance();
    const char *getName() { return "OPENING"; }
    virtual HandState *update(int pressure);
    virtual HandState *enter();
};

class HandStateHoldClosing : public HandState {
  public:
    static HandState& instance();
    const char *getName() { return "HOLD_CLOSING"; }
    virtual HandState *update(int pressure);
    virtual HandState *enter();
};

class HandStateClosing : public HandState {
  public:
    static HandState& instance();
    const char *getName() { return "CLOSING"; }
    virtual HandState *update(int pressure);
    virtual HandState *enter();
};

class HandStateIdleClosed : public HandState {
  public:
    static HandState& instance();
    const char *getName() { return "IDLE_CLOSED"; }
    virtual HandState *update(int pressure);
    virtual HandState *enter();
};

class HandStateClosed : public HandState {
  public:
    static HandState& instance();
    const char *getName() { return "CLOSED"; }
    virtual HandState *update(int pressure);
};

class HandStateOpen : public HandState {
  public:
    static HandState& instance();
    const char *getName() { return "OPEN"; }
    virtual HandState *update(int pressure);
    virtual HandState *enter();
};


#endif // HAND_STATE_H