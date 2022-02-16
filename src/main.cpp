#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <knx.h>
#include "forceSensor.h"
#include "log.h"
#include "parameters.h"
#include "states.h"

#define DEBUG
// create a pixel strand with 1 pixel on PIN_NEOPIXEL
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);
#define PIN_FORCE1 0
#define PIN_FORCE2 1
#define PIN_PROG_BUTTON 2
#define STATE_MEMORY_SIZE 20

uint32_t DeviceStartTimeInSeconds = 3;
uint32_t LifetickInSeconds = 60;
uint32_t SensorPollingIntervallInMs = 1000;


GroupObject *goLifeTickCounter = NULL;
GroupObject* goLifeTick = NULL;
GroupObject* goEnableDiagnostic = NULL;

ForceSensor *forceSensor1 = NULL;
ForceSensor *forceSensor2 = NULL;
ForceSensorSum *forceSensorSum = NULL;
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



uint8_t* saveState(uint8_t* buffer)
{
  if (forceSensor1 != NULL)
  {
    StateWriter stateWriter(buffer);

    forceSensor1->writeState(stateWriter);
    forceSensor2->writeState(stateWriter);
    forceSensorSum->writeState(stateWriter);

    int written = stateWriter.currentBuffer() - buffer;
    logValue("Flash", "Saved Bytes", written);
    if (written > STATE_MEMORY_SIZE && ArduinoPlatform::SerialDebug != NULL)
    {
      ArduinoPlatform::SerialDebug->print("ERROR: To many bytes written. Buffer ");
      ArduinoPlatform::SerialDebug->print(STATE_MEMORY_SIZE);
      ArduinoPlatform::SerialDebug->println(" to small");
      ArduinoPlatform::SerialDebug->println("Increase value of STATE_MEMORY_SIZE.");
    }
  }
  return buffer + STATE_MEMORY_SIZE;
}

const uint8_t* restoreState(const uint8_t* buffer)
{
  if (forceSensor1 != NULL)
  {
    StateReader stateReader(buffer);

    forceSensor1->readState(stateReader);
    forceSensor2->readState(stateReader);
    forceSensorSum->readState(stateReader);

    int read = stateReader.currentBuffer() - buffer;
    logValue("Flash", "Read Bytes", read);
    if (read > STATE_MEMORY_SIZE && ArduinoPlatform::SerialDebug != NULL)
    {
      ArduinoPlatform::SerialDebug->print("ERROR: To many bytes read. Buffer ");
      ArduinoPlatform::SerialDebug->print(STATE_MEMORY_SIZE);
      ArduinoPlatform::SerialDebug->println(" to small.");
      ArduinoPlatform::SerialDebug->println("Increase value of STATE_MEMORY_SIZE.");
    }
  }
  return buffer + STATE_MEMORY_SIZE;
}
void callback(GroupObject groupObject)
{
  if (goEnableDiagnostic == &groupObject)
  {
    diagnosticMode = goEnableDiagnostic->value();
  }
  forceSensor1->callback(groupObject);
  forceSensor2->callback(groupObject);
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
  knx.setSaveCallback(saveState);
  knx.setRestoreCallback(restoreState);
  knx.setProgLedCallbacks(progLedOff, progLedOn);
  knx.buttonPin(PIN_PROG_BUTTON);
  knx.readMemory();

  // print values of parameters if device is already configured
  if (knx.configured())
  {
    Serial.println("Initialize group objects");

    uint32_t parameterAddress = 0;
    
    DeviceStartTimeInSeconds = readKnxParameterUInt32("General", "DeviceStartTimeInSeconds", parameterAddress);
    LifetickInSeconds = readKnxParameterUInt32("General", "LifetickInSeconds", parameterAddress);
    SensorPollingIntervallInMs = readKnxParameterUInt32("General", "SensorPollingIntervallInMs", parameterAddress);

    int groupIndex = 1;
    goLifeTickCounter = &knx.getGroupObject(groupIndex++);
    goLifeTickCounter->dataPointType(DPT_Value_2_Count);
    goLifeTick = &knx.getGroupObject(groupIndex++);
    goLifeTick->dataPointType(DPT_Trigger);
   
    goEnableDiagnostic = &knx.getGroupObject(groupIndex++);
    goEnableDiagnostic->callback(callback);
    goEnableDiagnostic->dataPointType(DPT_Switch);

    forceSensor1 = new ForceSensor(PIN_FORCE1, "Force 1", groupIndex, parameterAddress, callback);
    forceSensor2 = new ForceSensor(PIN_FORCE1, "Force 2", groupIndex, parameterAddress, callback);
    
    knx.readMemory(); // read again to inialize sensors

    ForceSensor **allSensors = new ForceSensor *[2]
    { forceSensor1, forceSensor2 };
    forceSensorSum = new ForceSensorSum("Sum", groupIndex, parameterAddress, callback, allSensors, 2);

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
  if (!knx.configured())
    return;

  unsigned long now = millis();
  if (startTime == 0)
  {
    startTime = now; 
  }
  bool forceSent = false;
  if (!started)
  {
    if (now - startTime < DeviceStartTimeInSeconds * 1000)
      return;
    started = true;
    forceSent = true;
    Serial.println("Start device");
  }
  if (lastLoop == 0 || now - lastLoop >= SensorPollingIntervallInMs)
  {
    lastLoop = now;
    forceSensor1->loop(now, diagnosticMode, forceSent);
    forceSensor2->loop(now, diagnosticMode, forceSent);
    forceSensorSum->loop(now, diagnosticMode, forceSent);
  }
  if (forceSent || now - lastLifeTick >= LifetickInSeconds * 1000)
  {
    lastLifeTick = now;
    lifeTick++;
    logValue("Main", "Lifetick", lifeTick);
    goLifeTickCounter->value(lifeTick);
    goLifeTick->value(true);
  }
}