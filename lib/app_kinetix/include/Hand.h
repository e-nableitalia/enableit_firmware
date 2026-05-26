#pragma once
#include <enableit.h>
#include <driver/adc.h>

#include "Finger.h"
#include "FingerMovement.h"

#define FINGER_COUNT 5

/*
 *   Thumb pulley expects 120°, other pulleys expect 180°
 */

#ifdef ARDUINO_XIAO_ESP32S3
// Pinout for Seeed Studio XIAO boards
#define THUMB_CTRL_PIN D6
#define FINGER1_CTRL_PIN D10
#define FINGER2_CTRL_PIN D9
#define FINGER3_CTRL_PIN D8
#define FINGER4_CTRL_PIN D7
#else
#ifdef ARDUINO_M5Stack_ATOMS3
// Pinout for M5Stack AtomS3
#define THUMB_CTRL_PIN GPIO_NUM_7
#define FINGER1_CTRL_PIN GPIO_NUM_0
#define FINGER2_CTRL_PIN GPIO_NUM_1
#define FINGER3_CTRL_PIN GPIO_NUM_6
#define FINGER4_CTRL_PIN GPIO_NUM_5
#else
#error "Unsupported board: please define pin mappings for your board."
#endif
#endif

/* Fingers 3 and 5 servos are mounted in oposite direction so their min and max are reversed compared to other fingers. */
/* Left hand and right hand are symetrical so min and max need to be reversed */
/* servos homing is supposed to be done before wiring, when servos can go full range from 0 to 180° */

#ifdef LEFT_HAND
#define SERVO_CALIBRATION_OPEN 180
#define SERVO_CALIBRATION_CLOSED 0

#define THUMB_MAX_OPEN 120
#define THUMB_MAX_CLOSED 0

#define FINGER1_MAX_OPEN 0
#define FINGER1_MAX_CLOSED 140

#define FINGER2_MAX_OPEN 40
#define FINGER2_MAX_CLOSED 180

#define FINGER3_MAX_OPEN 140
#define FINGER3_MAX_CLOSED 0

#define FINGER4_MAX_OPEN 30
#define FINGER4_MAX_CLOSED 120
#else
#define SERVO_CALIBRATION_OPEN 0
#define SERVO_CALIBRATION_CLOSED 180

#define THUMB_MAX_OPEN 0
#define THUMB_MAX_CLOSED 120

#define FINGER1_MAX_OPEN 140
#define FINGER1_MAX_CLOSED 0

#define FINGER2_MAX_OPEN 180
#define FINGER2_MAX_CLOSED 40

#define FINGER3_MAX_OPEN 0
#define FINGER3_MAX_CLOSED 140

#define FINGER4_MAX_OPEN 120
#define FINGER4_MAX_CLOSED 30
#endif

class Hand {
public:
   Hand();

   Finger *fingers[5]; // 0 is thumb

   void close();
   void close(uint finger);
   void setCalibration(boolean);

   void open();
   void open(uint finger);

   bool isStill();
   bool isStill(uint finger);

   void setStep(float step);
   void setStep(int finger, float step);

   void setFinger(uint finger, FingerMovement *fingerMovement);
   void run();
   void run(uint finger);
   void stop();
   void stop(uint finger);
   void moveRelative(int to);
};