
#include "OptionalSensorProcessor.h"


RealSensorProcessor::RealSensorProcessor(int offset, int threshold) {
   this->offset = offset;
   this->threshold = threshold;
   analogSetPinAttenuation(SENSOR_PIN, ADC_6db);
   log_i("Real Sensor Processor created with offset=%d and threshold=%d", offset, threshold);
}

void RealSensorProcessor::setUpdateCallback(UpdateFn fn, void* ctx) {
   updateFn = fn;
   updateCtx = ctx;
}

void RealSensorProcessor::run() {
   uint16_t newReading = 0;
   // We must not read the ADC too fast
   if (millis() - lastMeasureAt > measureIntervalMs) {
      newReading = getAvg();
      lastMeasureAt = millis();
   } else {
      return;
   }

   // Let's read offset and threshold from all-purpose "experiment" settings
   int position = offset - (newReading/10);
   if (position < 0) position = 0;
   
   // We don't want to change position for tiny variation.
   // ADC is typically 1% accurate to readings vary even if voltage does not   
   if (abs(position - previousPosition) > threshold) {
      previousPosition = position;
      if (updateFn) {
         updateFn(position, updateCtx);
      }
   }

}

uint16_t RealSensorProcessor::getAvg() {
   uint16_t measure = analogRead(SENSOR_PIN);

   if (count == MAX_READINGS) {
      sum -= readings[index]; // Subtract oldest reading from total  
   }
   readings[index] = measure; // Store new reading
   sum += measure; // Add new reading to total
   index = (index + 1) % MAX_READINGS; // Increment index, loop if necessary
   if (count < MAX_READINGS) {
      count++;
   }
   return sum / count; // Return average.
}
