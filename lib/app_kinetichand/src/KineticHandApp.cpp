#include <enableit.h>
#include <Console.h>
#include <WiFi.h>
#include <Board.h>
#include <Display.h>
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

BOARDAPP_INSTANCE(KineticHandApp);

void KineticHandApp::enter() {
    log_d("Entering in KineticHand App state");
    display.clear();
    display.setTextSize(1);  // Set text size
    display.setTextColor(Display::Color::WHITE);  // Set text color to white
    handInit();
}
void KineticHandApp::leave() {
    log_d("Leaving KineticHand App state");
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

#ifdef ARDUINO_M5STACK_ATOMS3
    // Clear the screen and display pressure bar and battery voltage
    display.setTextColor(Display::Color::BLUE);
    //display.setRotation(3);
    display.setTextSize(2);
    display.fillScreen(Display::Color::BLACK);
    display.fillRect(10, 40, barLength, 10, Display::Color::GREEN);  // Draw the pressure bar
 
    display.setCursor(0,10);
    display.print("Pressure: ");
    display.println(pressureValue);
    display.print("\nServo: ");
    display.println(servo_angle);
#endif

    log_d("Pressure %d, Servo %d", pressureValue, servo_angle);
    
    delay(50);  // Delay for stability
}

const char *KineticHandApp::name() {
    return APP_KINETICHAND;
}
