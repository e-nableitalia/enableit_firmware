#pragma once

#include <enableit.h>
#include "Hand.h"
#include "Settings.h"

#ifdef ARDUINO_M5Stack_ATOMS3
// Pinout for M5Stack AtomS3
#define SENSOR_PIN GPIO_NUM_36
#else
#ifdef ARDUINO_XIAO_ESP32S3
#define SENSOR_PIN A0
#else
#error "Unsupported board: please define sensor pin for your board."
#endif
#endif
#define MAX_READINGS 5 // Number of readings to average

class SensorProcessor {
public:
   using UpdateFn = void (*)(int position, void* ctx);

   virtual void run() = 0;
   virtual uint16_t getAvg() = 0;

   // used to keep debounce logic decoupled from specific processing
   virtual void setUpdateCallback(UpdateFn fn, void* ctx) {
      updateFn = fn;
      updateCtx = ctx;
   }

   virtual const char *name() const { return "Generic"; }

protected:
   UpdateFn updateFn = nullptr;
   void* updateCtx = nullptr;
};

class RealSensorProcessor : public SensorProcessor {
public:

   RealSensorProcessor(int offset, int threshold);

   void run() override;
   uint16_t getAvg() override;

   virtual const char *name() const override { return "RealSensor"; }

   time_t lastMeasureAt = 0;
   time_t measureIntervalMs = 70; 
   uint16_t readings[MAX_READINGS]; // Array to store readings
   uint16_t previousPosition = 0;   
   int index = 0; // Index for the current reading
   long sum = 0;
   int count = 0;
   int offset;
   int threshold;
};


class MockSensorProcessor : public SensorProcessor {
public:
   MockSensorProcessor() {
      log_i("Mock Sensor Processor created");
   };
   void run() override {
      // Do nothing
   } 

   uint16_t getAvg() override {
      return 0;
   }

   virtual const char *name() const override { return "MockSensor"; }
};