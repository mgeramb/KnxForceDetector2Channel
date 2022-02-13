#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <knx.h>
#include "forceSensor.h"
#include "log.h"

#define DEBUG
// create a pixel strand with 1 pixel on PIN_NEOPIXEL
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);
#define PIN_FORCE1 0
#define PIN_FORCE2 1
#define PIN_PROG_BUTTON 2
#define DEVICE_START_TIME_MS 3000
#define SENSOR_POLLING_INTERVAL_MS 1000
#define LIFETICK_INTERVAL_MS 60000

GroupObject* goLifeTick;
GroupObject* goEnableDiagnostic;

ForceSensor forceSensor1 = ForceSensor(PIN_FORCE1, "Force 1");
ForceSensor forceSensor2 = ForceSensor(PIN_FORCE2, "Force 2");
unsigned long lastLoop = 0;
unsigned long lastLifeTick = 0;
unsigned long startTime = 0;
uint16_t lifeTick = 0;
bool diagnosticMode = true;
bool started = false;

void progLedOff()
{
  pixels.clear();
  pixels.show();
}

void progLedOn()
{
  pixels.setPixelColor(0, pixels.Color(20, 0, 0));
  pixels.show();
}

void callback(GroupObject groupObject)
{
  if (goEnableDiagnostic == &groupObject)
  {
    diagnosticMode = goEnableDiagnostic->value();
  }
  forceSensor1.callback(groupObject);
  forceSensor2.callback(groupObject);
}

void setup()
{
  pixels.begin();
#ifdef DEBUG
  delay(5000);
  Serial.begin(115200);
  Serial.println("Hello World!");
#endif
  ArduinoPlatform::SerialDebug = &Serial;
  knx.setProgLedCallbacks(progLedOff, progLedOn);
  knx.buttonPin(PIN_PROG_BUTTON);
  knx.readMemory();

  // print values of parameters if device is already configured
  if (knx.configured() || 1)
  {
    Serial.println("Initialize group objects");
 
    int groupIndex = 1;
    goLifeTick = &knx.getGroupObject(groupIndex++);
    goLifeTick->dataPointType(DPT_Value_2_Count);
    goEnableDiagnostic = &knx.getGroupObject(groupIndex++);
   //goEnableDiagnostic->callback(callback);
    goEnableDiagnostic->dataPointType(DPT_Switch);
    forceSensor1.initGroupObjects(groupIndex, callback);
    forceSensor2.initGroupObjects(groupIndex, callback);
    Serial.println("Group objects initialized");
  }
  else
  {
    Serial.println("Not yet configured");
  }

  Serial.println("Start framework");
  knx.start();
}

void loop()
{
  knx.loop();

  // only run the application code if the device was configured with ETS
 // if (!knx.configured())
  //  return;

  unsigned long now = millis();
  if (startTime == 0)
  {
    startTime = now; 
  }
  bool forceSent = false;
  if (!started)
  {
    if (now - startTime < DEVICE_START_TIME_MS)
      return;
    started = true;
    forceSent = true;
  }
  if (lastLoop == 0 || now - lastLoop >= SENSOR_POLLING_INTERVAL_MS)
  {
    lastLoop = now;
    forceSensor1.loop(now, diagnosticMode, forceSent);
    forceSensor2.loop(now, diagnosticMode, forceSent);
  }
  if (forceSent || now - lastLifeTick >= LIFETICK_INTERVAL_MS)
  {
    lastLifeTick = now;
    lifeTick++;
    log("Main", "Lifetick", lifeTick);
  }
}