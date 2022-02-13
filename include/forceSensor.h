#ifndef FORCESENSOR_H
#define FORCESENSOR_H

#include <knx.h>

class ForceSensorBase
{
protected:
  GroupObject goErrorForce;
  GroupObject goForce;
  GroupObject goForcePercentage;
  GroupObject goForceDetected;
  GroupObject goManualControlForceDetected;
  GroupObject goSetDetectionLimit;
  byte detectionLimit = 0;
  bool manualControlForceDetected;
  uint32_t lastRaw = 0;
  byte lastPercent = 0;
  bool lastError = 0;
  bool lastDetected;
  const char *name;
  ForceSensorBase(const char *name);

public:
  virtual void initGroupObjects(int& groupObjectIndex, GroupObjectUpdatedHandler callback);
  virtual void callback(GroupObject& groupObject);
};

class ForceSensor : ForceSensorBase
{
  GroupObject goSetLowerLimit;
  GroupObject goSetUpperLimit;
  uint16_t lowerLimit = 1;
  uint16_t upperLimit = 1022;
  uint32_t pin;

public:
  ForceSensor(uint32_t pin, const char *name);
  void loop(unsigned long now, bool diagosticMode, bool forceSent);
  virtual void initGroupObjects(int& groupObjectIndex, GroupObjectUpdatedHandler callback);
  virtual void callback(GroupObject& groupObject);
};

#endif