#include <Arduino.h>
#include <Console.h>
#include <WiFi.h>
#include <BleMouse.h>
#include <M5AtomS3.h>
#include <Config.h>
#include "PressureApp.h"

#ifdef ARDUINO_M5Stack_ATOMS3
#define PRESSURE_PIN    G5
#else
#define PRESSURE_PIN    GPIO_NUM_8
#pragma warning "STUB Pressure PIN defined"
#endif

#define PRESSURE_MAX    4096

BleMouse bleMouse;
const int pressureThreshold = 1000;  // Adjust this threshold according to your sensor

void PressureApp::enter() {
    DBG("Entering in Pressure App state");
#ifdef ARDUINO_M5Stack_ATOMS3
    m5.Lcd.clear();
    m5.Lcd.setTextSize(1);  // Set text size
    m5.Lcd.setTextColor(TFT_WHITE);  // Set text color to white
#endif
//    bleMouse.begin();
    pinMode(PRESSURE_PIN, INPUT);
}
void PressureApp::leave() {
    DBG("Leaving boot Pressure App state");
}

void PressureApp::process() {
    int pressureValue = PRESSURE_MAX - analogRead(PRESSURE_PIN);
  
    // Map pressureValue to display bar length
    int barLength = map(pressureValue, 0, 4096, 0, 120);

//    if (pressureValue > pressureThreshold) {
//        bleMouse.click(MOUSE_LEFT);  // Emulate left mouse button click
//        delay(50);  // Delay to prevent rapid multiple clicks
//    }

#ifdef ARDUINO_M5Stack_ATOMS3
    // Clear the screen and display pressure bar and battery voltage
    m5.Lcd.setTextColor(BLUE);
    //m5.Lcd.setRotation(3);
    m5.Lcd.setTextSize(2);
    m5.Lcd.fillScreen(TFT_BLACK);
    m5.Lcd.fillRect(10, 40, barLength, 10, TFT_GREEN);  // Draw the pressure bar
 
    m5.Lcd.setCursor(0,10);
    m5.Lcd.print("Pressure: ");
    m5.Lcd.println(pressureValue);
#endif
    DBG("Pressure %d", pressureValue);
    
    delay(50);  // Delay for stability
}

const char *PressureApp::name() {
    return APP_PRESSURE;
}
