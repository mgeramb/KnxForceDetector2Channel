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

enum ErrorMode
{
  SwitchOff = 0,
  SwitchOn = 1,
  SwitchOffForUnderflowOnForOverflow = 2,
  KeepLastValidState = 3,
};

enum DelayedSwitchOffTime
{
  OneSecond = 0,
  TenSeconds = 1,
  ThirtySeconds = 2,
  OneMinute = 3,
  TwoMinute = 4,
  FiveMinutes = 5,
  TenMinutes = 6,
  FifteenMinutes = 7,
  ThirtyMinutes = 8,
  OneHour = 9,
  TwoHours = 10,
  TreeHours = 11,
  FourHours = 12,
  FiveHours = 13,
  SixHours = 14,
  SevenHours = 15,
  EightHours = 16,
  NineHours = 17,
  TenHours = 18,
  OnlySendOn = 19,
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
  GroupObject& goSwitchDelayedOff;
  GroupObject& goSetDetectionLimit;
private:
  byte detectionLimit = 0;
  uint32_t lastRaw = 0;
  byte lastPercent = 0;
  bool lastError = 0;
  byte hysterese;
  byte percentChangeToSent = 1;
  bool lastForceDetected;
  bool lockForceDetected = 0;
  LockMode lockModeForceDetected;
  ErrorMode errorMode;
private:
  uint32_t delayedOffTime = DelayedSwitchOffTime::TenHours;
  uint32_t delayedOffTimer = 0;
  bool lastSwitchDelayed = 0;
protected:
  ForceSensorBase(const char *name, int &groupObjectIndex, uint32_t &parameterAddress, GroupObjectUpdatedHandler callback);
  void HandleDelayedOff(const char *operation, uint32_t now, bool forceSent, uint32_t delayedOffTime, uint32_t delayedOffTimer, bool &lastState, bool newState, GroupObject &go);
  static uint32_t getDelayedOffTime(DelayedSwitchOffTime delayedOffTime);

public:
  virtual uint32_t getLowerLimit() = 0;
  virtual uint32_t getUpperLimit() = 0;
  virtual uint32_t getRaw() = 0;
  bool getLastDetected();
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
  virtual void writeState(StateWriter& stateWriter);
  virtual void readState(StateReader &stateReader);
  virtual void callback(GroupObject& groupObject);
  virtual uint32_t getLowerLimit();
  virtual uint32_t getUpperLimit();
  virtual uint32_t getRaw();
};

class ForceSensorSum : public ForceSensorBase
{
  ForceSensor** sensors;
  size_t sensorCount;
  GroupObject& goDetectedAny;
  GroupObject& goLockDetectedAny;
  GroupObject &goSwitchDelayedOffAny;
  bool lastAnyDetected = false;
  bool lockAnyDetected;
  LockMode lockModeAnyDetected;
  uint32_t delayedOffTimeAny = DelayedSwitchOffTime::TenHours;
  uint32_t delayedOffTimerAny = 0;
  bool lastSwitchDelayedAny = 0;
public:
  ForceSensorSum(const char* name, int& groupObjectIndex, uint32_t& parameterAddress, GroupObjectUpdatedHandler callback, ForceSensor** sensors, size_t sensorCount);
  virtual void callback(GroupObject& groupObject);
  virtual void readState(StateReader &stateReader);
  virtual void writeState(StateWriter& stateWriter);
  virtual void loop(unsigned long now, bool diagosticMode, bool forceSent);
  virtual uint32_t getLowerLimit();
  virtual uint32_t getUpperLimit();
  virtual uint32_t getRaw();
};


#endif