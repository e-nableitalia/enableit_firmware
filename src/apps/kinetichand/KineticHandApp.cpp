#include <Arduino.h>
#include <Console.h>
#include <WiFi.h>
#include <M5AtomS3.h>
#include <Config.h>
#include "KineticHandApp.h"
#include "HandState.h"

const int pressureThreshold = 1000;  // Adjust this threshold according to your sensor
// default, updated by EEPROM
int pressureValue;
int setpoint = 2048;  
int servo_angle = 0;
// hand mode
int mode = MODE_PROGRESSIVE;

void KineticHandApp::enter() {
    DBG("Entering in KineticHand App state");
#ifdef ARDUINO_M5Stack_ATOMS3
    m5.Lcd.clear();
    m5.Lcd.setTextSize(1);  // Set text size
    m5.Lcd.setTextColor(TFT_WHITE);  // Set text color to white
#endif
    handInit();
}
void KineticHandApp::leave() {
    DBG("Leaving KineticHand App state");
}

void KineticHandApp::process() {

    if (mode == MODE_PROGRESSIVE)
        updateServoProgressive();
    
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
    m5.Lcd.print("\nServo: ");
    m5.Lcd.println(servo_angle);
#endif

    DBG("Pressure %d, Servo %d", pressureValue, servo_angle);
    
    delay(50);  // Delay for stability
}

const char *KineticHandApp::name() {
    return APP_KINETICHAND;
}
