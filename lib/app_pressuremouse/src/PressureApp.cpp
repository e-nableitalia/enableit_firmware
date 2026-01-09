#include <Arduino.h>
#include <Console.h>
#include <WiFi.h>
#include <BleMouse.h>
#include <Display.h>
#include <Config.h>
#include "PressureApp.h"

#ifdef ENABLEIT_BOARD_M5STACK_ATOMS3
#define PRESSURE_PIN    G5
#else
#define PRESSURE_PIN    GPIO_NUM_8
#pragma warning "STUB Pressure PIN defined"
#endif

#define PRESSURE_MAX    4096

BleMouse bleMouse;
const int pressureThreshold = 1000;  // Adjust this threshold according to your sensor

BOARDAPP_INSTANCE(PressureApp);

void PressureApp::enter() {
    log_d("Entering in Pressure App state");
    display.clear();
    display.setTextSize(1);  // Set text size
    display.setTextColor(Display::Color::WHITE);  // Set text color to white
//    bleMouse.begin();
    pinMode(PRESSURE_PIN, INPUT);
}
void PressureApp::leave() {
    log_d("Leaving boot Pressure App state");
}

void PressureApp::process() {
    int pressureValue = PRESSURE_MAX - analogRead(PRESSURE_PIN);
  
    // Map pressureValue to display bar length
    int barLength = map(pressureValue, 0, 4096, 0, 120);

//    if (pressureValue > pressureThreshold) {
//        bleMouse.click(MOUSE_LEFT);  // Emulate left mouse button click
//        delay(50);  // Delay to prevent rapid multiple clicks
//    }

    // Clear the screen and display pressure bar and battery voltage
    display.setTextColor(Display::Color::BLUE);
    //display.setRotation(3);
    display.setTextSize(2);
    display.fillScreen(Display::Color::BLACK);
    display.fillRect(10, 40, barLength, 10, Display::Color::GREEN);  // Draw the pressure bar
 
    display.setCursor(0,10);
    display.print("Pressure: ");
    display.println(pressureValue);
    log_d("Pressure %d", pressureValue);
    
    delay(50);  // Delay for stability
}

const char *PressureApp::name() {
    return APP_PRESSURE;
}
