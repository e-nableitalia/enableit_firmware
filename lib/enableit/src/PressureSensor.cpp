#if 0

#include <M5StickC.h>
#include <BleMouse.h>

BleMouse bleMouse;
const int pressurePin = 36;  // Analog pin for pressure sensor on M5Stick
const int pressureThreshold = 500;  // Adjust this threshold according to your sensor

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(1);  // Set text size
  M5.Lcd.setTextColor(TFT_WHITE);  // Set text color to white
  bleMouse.begin();
  pinMode(pressurePin, INPUT);
}

void loop() {
  M5.update();

  int pressureValue = analogRead(pressurePin);
  
  // Map pressureValue to display bar length
  int barLength = map(pressureValue, 0, 1023, 0, 120);

  if (pressureValue > pressureThreshold) {
    bleMouse.click(MOUSE_LEFT);  // Emulate left mouse button click
    delay(200);  // Delay to prevent rapid multiple clicks
  }

  // Clear the screen and display pressure bar and battery voltage
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.fillRect(10, 40, barLength, 10, TFT_GREEN);  // Draw the pressure bar
  
  float batteryVoltage = M5.Axp.GetBatVoltage() / 1.0;
  M5.Lcd.drawString("Pressure", 10, 10);
  M5.Lcd.drawString(" " + String(batteryVoltage) + " V", 10, 60);

  delay(80);  // Delay for stability
}

#endif