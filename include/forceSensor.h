#ifndef FORCESENSOR_H
#define FORCESENSOR_H

#include <knx.h>

class ForceSensorBase
{
protected:
  const char *name;
  GroupObject& goErrorForce;
  GroupObject& goForce;
  GroupObject& goForcePercentage;
  GroupObject& goForceDetected;
  GroupObject& goManualControlForceDetected;
  GroupObject& goSetDetectionLimit;
  byte detectionLimit = 0;
  bool manualControlForceDetected;
  uint32_t lastRaw = 0;
  byte lastPercent = 0;
  bool lastError = 0;
  bool lastDetected;
  ForceSensorBase(const char *name, int& groupObjectIndex, GroupObjectUpdatedHandler callback);

public:
  virtual void callback(GroupObject& groupObject);
};

class ForceSensor : ForceSensorBase
{
  uint32_t pin;
  GroupObject& goSetLowerLimit;
  GroupObject& goSetUpperLimit;
  uint16_t lowerLimit = 1;
  uint16_t upperLimit = 1022;

public:
  ForceSensor(uint32_t pin, const char *name, int& groupObjectIndex, GroupObjectUpdatedHandler callback);
  void loop(unsigned long now, bool diagosticMode, bool forceSent);
  virtual void callback(GroupObject& groupObject);
};

#endif