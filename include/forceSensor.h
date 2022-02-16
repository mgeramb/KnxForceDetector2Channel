#ifndef FORCESENSOR_H
#define FORCESENSOR_H

#include <knx.h>
#include "states.h"

enum ManualControl
{
  NoForceOff = 0,
  NoForceOn = 1,
  ForceOff = 2,
  ForceOn = 3
};

class ForceSensorBase
{
protected:
  const char* name;
  GroupObject& goErrorForce;
  GroupObject& goForce;
  GroupObject& goForcePercentage;
  GroupObject& goForceDetected;
  GroupObject& goManualControlForceDetected;
  GroupObject& goSetDetectionLimit;
  byte detectionLimit = 0;
  ManualControl manualControlForceDetected;
  uint32_t lastRaw = 0;
  byte lastPercent = 0;
  bool lastError = 0;
  bool lastDetected;
  byte hysterese;
  ForceSensorBase(const char* name, int& groupObjectIndex, uint32_t& parameterAddress, GroupObjectUpdatedHandler callback);
public:
  virtual uint32_t getLowerLimit() = 0;
  virtual uint32_t getUpperLimit() = 0;
  virtual uint32_t getRaw() = 0;
  virtual bool getLastDetected();
  virtual void callback(GroupObject &groupObject);
  virtual void writeState(StateWriter& stateWriter);
  virtual void readState(StateReader& stateReader);
  virtual void loop(unsigned long now, bool diagosticMode, bool forceSent);
};

class ForceSensor : public ForceSensorBase
{
  uint32_t pin;
  GroupObject& goSetLowerLimit;
  GroupObject& goSetUpperLimit;
  uint16_t lowerLimit = 1;
  uint16_t upperLimit = 1022;

public:
  virtual uint32_t getLowerLimit();
  virtual uint32_t getUpperLimit();
  virtual uint32_t getRaw();
  virtual void writeState(StateWriter& stateWriter);
  virtual void readState(StateReader &stateReader);
  ForceSensor(uint32_t pin, const char *name, int &groupObjectIndex, uint32_t &parameterAddress, GroupObjectUpdatedHandler callback);
  virtual void callback(GroupObject& groupObject);
};

class ForceSensorSum : public ForceSensorBase
{
  ForceSensor** sensors;
  size_t sensorCount;
  GroupObject& goDetectedAny;
  bool lastAnyDetected = false;

public:
  virtual uint32_t getLowerLimit();
  virtual uint32_t getUpperLimit();
  virtual uint32_t getRaw();
  ForceSensorSum(const char* name, int& groupObjectIndex, uint32_t& parameterAddress, GroupObjectUpdatedHandler callback, ForceSensor** sensors, size_t sensorCount);
  virtual void loop(unsigned long now, bool diagosticMode, bool forceSent);
};


#endif