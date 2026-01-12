#include <Arduino.h>

#ifdef ENABLEIT_BOARD_XIAO_ESP32S3
#include <BoardManager.h>
#endif

#include "OptionalSensorProcessor.h"

#include "Hand.h"
#include "HandMovementFactory.h"
#include "Sequence.h"

#if defined DEMO
#define NEEDSEQ
#endif


SensorProcessor *sensorProcessor = NULL;

int start = 0;
int finger = 0;
bool isClosed = true;

Hand *hand = new Hand();
HandMovementFactory *hmf = new HandMovementFactory(hand);

Sequence *seq = NULL;
Settings *settings;

//ENABLE_BOARD_APP(BootLoaderApp);

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Setup");
  JsonDocument systemInfo;
  // Set an option array
  JsonArray options = systemInfo.createNestedArray("options");

  #ifdef GIT_REV
  log_i("Version %s\n", GIT_REV);
  systemInfo["git_rev"] = GIT_REV;
  #endif 

  Serial.println("Delaying for 3s...");
  delay(3000);

  #ifdef ENABLEIT_BOARD_XIAO_ESP32S3
  //ENABLEIT_BOOT(APP_BOOT);
  Serial.println("Board initialized");
  delay(1000);
  #endif

  #ifdef CORE_DEBUG_LEVEL
  options.add("CORE_DEBUG_LEVEL=" + String(CORE_DEBUG_LEVEL));
  #endif

  #ifdef LEFT_HAND
  options.add("LEFT_HAND");
  #else
  options.add("RIGHT_HAND");
  #endif

  // #ifdef WITH_OLED_DISPLAY
  // display = new RealDisplay();
  // options.add("OLED_DISPLAY");
  // #else
  // display = new MockDisplay();
  // #endif

  // display->setTitle("KinetiX");

  start = millis();
  isClosed = true;
 
  settings = new Settings();

  #ifdef WITH_SENSOR
  sensorProcessor = new RealSensorProcessor(hand, settings, display);
  options.add("SENSOR");
  #else
  sensorProcessor = new MockSensorProcessor();
  #endif

  // messageProcessor = new MessageProcessor(hand, settings, display, systemInfo);

  String json;
  serializeJson(systemInfo, json);
  log_i("System info: %s", json.c_str());

  // btServer = new BtServer(messageProcessor, display);
  #ifndef NEEDSEQ
  // Initialization sequence, do it just once
  seq = new Sequence(1); // this sequence runs just once
  seq->addMovement(hmf->five());
  seq->addMovement(hmf->half());
  seq->addMovement(hmf->five());
  log_i("Running init sequence");
  seq->start();   
  #endif

  #ifdef DEMO
  seq = new Sequence(0); // 0 is repeat forever
  seq->addMovement(hmf->openPinch(), 4000);
  seq->addMovement(hmf->one());
  seq->addMovement(hmf->two());
  seq->addMovement(hmf->three());
  seq->addMovement(hmf->four());
  seq->addMovement(hmf->five());
  seq->start();
  #endif

}
 
void loop() {
  // #ifndef NEEDSEQ
  //   messageProcessor->run();
  // #endif

  if ((seq == NULL || !seq->isRunning())
         /* && messageProcessor->isIdle() */) {
    sensorProcessor->run();
  }

  if (seq != NULL) {
    seq->run();
  }
  // display->refresh();

  //ENABLEIT_LOOP();
}
