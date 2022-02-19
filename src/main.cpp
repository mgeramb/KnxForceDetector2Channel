#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <knx.h>
#include <FlashAsEEPROM.h>
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

uint32_t DeviceStartTimeInSeconds = 3;
uint32_t LifetickInSeconds = 60;
uint32_t SensorPollingIntervallInMs = 1000;

GroupObject *goLifeTickCounter = NULL;
GroupObject *goLifeTick = NULL;
GroupObject *goEnableDiagnostic = NULL;

ForceSensor *forceSensor1 = NULL;
ForceSensor *forceSensor2 = NULL;
ForceSensorSum *forceSensorSum = NULL;
unsigned long lastLoop = 0;
unsigned long lastLifeTick = 0;
unsigned long startTime = 0;
uint16_t lifeTick = 0;
bool diagnosticMode = false;
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

void saveStates()
{
  uint8_t* eEPROMBuffer = EEPROM.getDataPtr();
  uint8_t *buffer = eEPROMBuffer + KNX_FLASH_SIZE;

  StateWriter stateWriter(buffer);

  forceSensor1->writeState(stateWriter);
  forceSensor2->writeState(stateWriter);
  forceSensorSum->writeState(stateWriter);

  int written = stateWriter.currentBuffer() - buffer;
  logValue("Flash", "Saved Bytes", written);
  if (written > EEPROM_EMULATION_SIZE - KNX_FLASH_SIZE && ArduinoPlatform::SerialDebug != NULL)
  {
    ArduinoPlatform::SerialDebug->print("ERROR: To many bytes written. Buffer ");
    ArduinoPlatform::SerialDebug->print(KNX_FLASH_SIZE);
    ArduinoPlatform::SerialDebug->println(" to large.");
    ArduinoPlatform::SerialDebug->print("Decrease value of KNX_FLASH_SIZE to");
    ArduinoPlatform::SerialDebug->print(EEPROM_EMULATION_SIZE - written);
    ArduinoPlatform::SerialDebug->println(".");
  }
  EEPROM.commit();
}

void restoreStates()
{
  uint8_t *eEPROMBuffer = EEPROM.getDataPtr();
  const uint8_t *buffer = eEPROMBuffer + KNX_FLASH_SIZE;
  StateReader stateReader(buffer);

  forceSensor1->readState(stateReader);
  forceSensor2->readState(stateReader);
  forceSensorSum->readState(stateReader);

  int read = stateReader.currentBuffer() - buffer;
  logValue("Flash", "Read Bytes", read);
  if (read > EEPROM_EMULATION_SIZE - KNX_FLASH_SIZE && ArduinoPlatform::SerialDebug != NULL)
  {
    ArduinoPlatform::SerialDebug->print("ERROR: To many bytes written. Buffer ");
    ArduinoPlatform::SerialDebug->print(KNX_FLASH_SIZE);
    ArduinoPlatform::SerialDebug->println(" to large.");
    ArduinoPlatform::SerialDebug->print("Decrease value of KNX_FLASH_SIZE to");
    ArduinoPlatform::SerialDebug->print(EEPROM_EMULATION_SIZE - read);
    ArduinoPlatform::SerialDebug->println(".");
  }
}
void callback(GroupObject groupObject)
{
  if (ArduinoPlatform::SerialDebug != NULL)
  {
    ArduinoPlatform::SerialDebug->print("Callback received for ");
    ArduinoPlatform::SerialDebug->println(groupObject.asap());
  }
  if (goEnableDiagnostic->asap() == groupObject.asap())
  {
    diagnosticMode = goEnableDiagnostic->value();
    logValue("Global", "EnableDiagnostic mode received", diagnosticMode);
  }
  forceSensor1->callback(groupObject);
  forceSensor2->callback(groupObject);
  forceSensorSum->callback(groupObject);
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
  knx.setProgLedOffCallback(progLedOff);
  knx.setProgLedOnCallback(progLedOn);
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

    forceSensorSum = new ForceSensorSum("Sum", groupIndex, parameterAddress, callback, new ForceSensor *[2]
    { forceSensor1, forceSensor2 }, 2);

    Serial.print(groupIndex - 1);
    Serial.println(" group objects initialized");

    restoreStates();
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
  if (StateWriter::CheckSaveNeededAndResetRequest(now))
  {
    saveStates();
  }
}