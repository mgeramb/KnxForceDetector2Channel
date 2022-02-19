#ifndef FORCESENSOR_H
#define FORCESENSOR_H

#include <knx.h>
#include "states.h"

enum LockMode
{
  SwitchToOff = 0,
  SwitchToOn = 1,
  LockCurrentState = 2,
};

class ForceSensorBase
{
protected:
  const char* name;
  GroupObject& goErrorForce;
  GroupObject& goForce;
  GroupObject& goForcePercentage;
  GroupObject& goForceDetected;
  GroupObject& goLockForceDetected;
  GroupObject& goSetDetectionLimit;
  byte detectionLimit = 0;
  uint32_t lastRaw = 0;
  byte lastPercent = 0;
  bool lastError = 0;
  byte hysterese;
  byte percentChangeToSent = 1;
  bool lastForceDetected;
  bool lockForceDetected = 0;
  LockMode lockModeForceDetected;
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
  ForceSensor(uint32_t pin, const char *name, int &groupObjectIndex, uint32_t &parameterAddress, GroupObjectUpdatedHandler callback);
  virtual uint32_t getLowerLimit();
  virtual uint32_t getUpperLimit();
  virtual uint32_t getRaw();
  virtual void writeState(StateWriter& stateWriter);
  virtual void readState(StateReader &stateReader);
  virtual void callback(GroupObject& groupObject);
};

class ForceSensorSum : public ForceSensorBase
{
  ForceSensor** sensors;
  size_t sensorCount;
  GroupObject& goDetectedAny;
  GroupObject& goLockDetectedAny;
  bool lastAnyDetected = false;
  bool lockAnyDetected;
  LockMode lockModeAnyDetected;
public:
  ForceSensorSum(const char* name, int& groupObjectIndex, uint32_t& parameterAddress, GroupObjectUpdatedHandler callback, ForceSensor** sensors, size_t sensorCount);
  virtual uint32_t getLowerLimit();
  virtual uint32_t getUpperLimit();
  virtual uint32_t getRaw();
  virtual void loop(unsigned long now, bool diagosticMode, bool forceSent);
  virtual void writeState(StateWriter& stateWriter);
  virtual void readState(StateReader &stateReader);
  virtual void callback(GroupObject& groupObject);
};


#endif