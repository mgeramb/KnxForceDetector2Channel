#include <knx.h>
#include "forceSensor.h"
#include "log.h"
#include "parameters.h"

ForceSensorBase::ForceSensorBase(const char *name, int &groupObjectIndex, uint32_t &parameterAddress, GroupObjectUpdatedHandler callback)
    : name(name),
      goErrorForce(knx.getGroupObject(groupObjectIndex++)),
      goForce(knx.getGroupObject(groupObjectIndex++)),
      goForcePercentage(knx.getGroupObject(groupObjectIndex++)),
      goForceDetected(knx.getGroupObject(groupObjectIndex++)),
      goLockForceDetected(knx.getGroupObject(groupObjectIndex++)),
      goSwitchDelayedOff(knx.getGroupObject(groupObjectIndex++)),
      goSetDetectionLimit(knx.getGroupObject(groupObjectIndex++)),
      hysterese(readKnxParameterUInt32(name, "Hysterese", parameterAddress)),
      percentChangeToSent((byte)readKnxParameterUInt32(name, "PercentChangeToSent", parameterAddress)),
      errorMode((ErrorMode)readKnxParameterUInt8(name, "ErrorMode", parameterAddress)),
      lockModeForceDetected((LockMode)readKnxParameterUInt8(name, "LockModeForceDetected", parameterAddress)),
      delayedOffTime(getDelayedOffTime((DelayedSwitchOffTime)readKnxParameterUInt8(name, "SwitchOffTime", parameterAddress)))
{
    Serial.println("initGroupObjects base");
    goErrorForce.dataPointType(DPT_Switch);
    goForce.dataPointType(DPT_Value_2_Ucount);
    goForcePercentage.dataPointType(DPT_Scaling);
    goForceDetected.dataPointType(DPT_Switch);
    goLockForceDetected.callback(callback);
    goLockForceDetected.dataPointType(DPT_Enable);
    goSetDetectionLimit.callback(callback);
    goSetDetectionLimit.dataPointType(DPT_Scaling);
    goSwitchDelayedOff.dataPointType(DPT_Switch);
    goSwitchDelayedOff.callback(callback);
    if (percentChangeToSent < 1)
        percentChangeToSent = 1;
}

uint32_t ForceSensorBase::getDelayedOffTime(DelayedSwitchOffTime delayedOffTime)
{
    switch(delayedOffTime)
    {
        case OneSecond:
            return 1000;
        case TenSeconds:
            return 1000 * 10;
        case ThirtySeconds:
            return 1000 * 30;
        case OneMinute:
            return 1000 * 60;
        case TwoMinute:
            return 1000 * 60 * 2;
        case FiveMinutes:
            return 1000 * 60 * 5;
        case TenMinutes:
            return 1000 * 60 * 10;
        case FifteenMinutes:
            return 1000 * 60 * 15;
        case ThirtyMinutes:
            return 1000 * 60 * 30;
        case OneHour:
            return 1000 * 3600;
        case TwoHours:
            return 1000 * 3600 * 2;
        case TreeHours:
            return 1000 * 3600 * 3;
        case FourHours:
            return 1000 * 3600 * 4;
        case FiveHours:
            return 1000 * 3600 * 5;
        case SixHours:
            return 1000 * 3600 * 6;
        case SevenHours:
            return 1000 * 3600 * 7;
        case EightHours:
            return 1000 * 3600 * 8;
        case NineHours:
            return 1000 * 3600 * 9;
        case TenHours:
            return 1000 * 3600 * 10;
        case OnlySendOn:
            return 0;
        default:
            return 0;
        }
}

void ForceSensorBase::callback(GroupObject &groupObject)
{
    if (goSetDetectionLimit.asap() == groupObject.asap())
    {
        detectionLimit = goSetDetectionLimit.value();
        logValue(name, "Detection limit received", detectionLimit);
        StateWriter::RequestSave();
    }
    else if (goLockForceDetected.asap() == groupObject.asap())
    {
        lockForceDetected = goLockForceDetected.value();
        logValue(name, "Lock force detected received", lockForceDetected);
        StateWriter::RequestSave();
    }
}

void ForceSensorBase::writeState(StateWriter &stateWriter)
{
    stateWriter.writeByte(detectionLimit);
    stateWriter.writeByte(lockForceDetected ? 1 : 0);
}

void ForceSensorBase::readState(StateReader &stateReader)
{
    detectionLimit = stateReader.readByte();
    if (detectionLimit == 0xFF)
    {
        detectionLimit = 50;
        goSetDetectionLimit.requestObjectRead();
    }
    goSetDetectionLimit.valueNoSend(detectionLimit);
    byte rowValue = stateReader.readByte();
    if (rowValue == 0xFF)
    {
        lockForceDetected = false;
        goSetDetectionLimit.requestObjectRead();
    }
    else if (rowValue == 1)
        lockForceDetected = true;
    else
        lockForceDetected = false;
    goLockForceDetected.valueNoSend(lockForceDetected);
}

void ForceSensorBase::loop(unsigned long now, bool diagnosticMode, bool forceSent)
{
    uint32_t raw = getRaw();
    if (diagnosticMode)
        logValue(name, "Read RAW", raw);
    uint32_t lowerLimit = getLowerLimit();
    uint32_t upperLimit = getUpperLimit();
    bool detected = false;
    bool error = false;
    byte percent = 0;
    if (raw < lowerLimit)
    {
        error = true;
        switch (errorMode)
        {
            case ErrorMode::SwitchOff:
                detected = false;
                break;
            case ErrorMode::SwitchOn:
                detected = true;
                break;
            case ErrorMode::SwitchOffForUnderflowOnForOverflow:
                detected = false;
                break;
            case ErrorMode::KeepLastValidState:
                detected = lastForceDetected;
                break;
        }
        percent = 0;
    }
    else if (raw > upperLimit)
    {
        error = true;
        switch (errorMode)
        {
            case ErrorMode::SwitchOff:
                detected = false;
                break;
            case ErrorMode::SwitchOn:
                detected = true;
                break;
            case ErrorMode::SwitchOffForUnderflowOnForOverflow:
                detected = true;
                break;
            case ErrorMode::KeepLastValidState:
                detected = lastForceDetected;
                break;
        }
        percent = 100;
    }
    else
    {
        float onePercent = ((float)(upperLimit - lowerLimit)) / (float)100;
        if (onePercent == 0)
            onePercent = 0.00001;
        percent = ((float)(raw - lowerLimit) / onePercent);
        if (diagnosticMode)
            logValue(name, "Detection Limit", detectionLimit);
        float currentDetectionLimit = detectionLimit;
        if (diagnosticMode)
            logValue(name, "Current Detection Limit before", currentDetectionLimit);
        if (diagnosticMode)
            logValue(name, "Hysterese", (float) hysterese);
        if (lastForceDetected)
        {
            if (currentDetectionLimit > hysterese)
                currentDetectionLimit -= (float)hysterese;
            else 
                currentDetectionLimit = 0;
        }
        if (diagnosticMode)
            logValue(name, "Current Detection Limit", currentDetectionLimit);
        if (percent >= currentDetectionLimit)
            detected = true;
    }
    if (diagnosticMode)
        logValue(name, "Percent", percent);
  
    if (lockForceDetected)
    {
        switch (lockModeForceDetected)
        {
            case LockMode::LockCurrentState:
                detected = lastForceDetected;
                break;
            case LockMode::SwitchToOff:
                detected = false;
                break;
            case LockMode::SwitchToOn:
                detected = true;
                break;
        }
    }
    if (!diagnosticMode)
        raw = 0;
    if (lastRaw != raw || forceSent)
    {
        lastRaw = raw;
        logValue(name, "Send force", lastRaw);
        goForce.value(lastRaw);
    }
    if (abs((int)lastPercent - (int)percent) >= (int)percentChangeToSent || // check for enough change
        forceSent ||                                                        // check for force
        lastForceDetected != detected ||                                         // check if detected changed
        (lastPercent != percent && (percent == 0 || percent == 100)))       // check if one limit was reached
    {
        lastPercent = percent;
        logValue(name, "Send percent", lastPercent);
        goForcePercentage.value(lastPercent);
    }
    if (lastError != error || forceSent)
    {
        lastError = error;
        logValue(name, "Send error", lastError);
        goErrorForce.value(lastError);
    }
    if (lastForceDetected != detected || forceSent)
    {
        lastForceDetected = detected;
        logValue(name, "Send force detected", lastForceDetected);
        goForceDetected.value(detected);
        delayedOffTimer = now;
    }
    HandleDelayedOff("Send switchDelayedOff", now, forceSent, delayedOffTime, delayedOffTimer, lastSwitchDelayed, detected, goSwitchDelayedOff);
}

void ForceSensorBase::HandleDelayedOff(const char* operation, uint32_t now, bool forceSent, uint32_t delayedOffTime, uint32_t delayedOffTimer, bool& lastState, bool newState, GroupObject& go)
{
    if (!newState)
    {
        if (delayedOffTime > 0)
        {
            // handle delayed of
            if (lastState && now - delayedOffTimer < delayedOffTime)
            {
                newState = true;
            }
        }
    }
    if (lastState != newState || forceSent)
    {
        lastState = newState;
        if (delayedOffTime > 0 || newState)
        {
            logValue(name, operation, newState);
            go.value(newState);
        }
    }
}

bool ForceSensorBase::getLastDetected()
{
    return lastForceDetected;
}

ForceSensor::ForceSensor(uint32_t pin, const char *name, int &groupObjectIndex, uint32_t &parameterAddress, GroupObjectUpdatedHandler callback) : 
    ForceSensorBase(name, groupObjectIndex, parameterAddress, callback),
    pin(pin),
    goSetLowerLimit(knx.getGroupObject(groupObjectIndex++)),
    goSetUpperLimit(knx.getGroupObject(groupObjectIndex++))
{
    Serial.println("initGroupObjects");
    goSetLowerLimit.callback(callback);
    goSetLowerLimit.dataPointType(DPT_Value_2_Ucount);
    goSetUpperLimit.callback(callback);
    goSetUpperLimit.dataPointType(DPT_Value_2_Ucount);
}

void ForceSensor::callback(GroupObject &groupObject)
{
    ForceSensorBase::callback(groupObject);
    if (goSetLowerLimit.asap() == groupObject.asap())
    {
        lowerLimit = goSetLowerLimit.value();
        logValue(name, "Lower limit received", lowerLimit);
        StateWriter::RequestSave();
    }
    else if (goSetUpperLimit.asap() == groupObject.asap())
    {
        upperLimit = goSetUpperLimit.value();
        logValue(name, "Upper limit received", upperLimit);
        StateWriter::RequestSave();
    }
}

void ForceSensor::writeState(StateWriter &stateWriter)
{
    ForceSensorBase::writeState(stateWriter);
    stateWriter.writeWord(lowerLimit);
    stateWriter.writeWord(upperLimit);
}

void ForceSensor::readState(StateReader &stateReader)
{
    ForceSensorBase::readState(stateReader);
    lowerLimit = stateReader.readWord();
    if (lowerLimit == 0xFFFF)
    {
        lowerLimit = 1;
        goSetLowerLimit.requestObjectRead();
    }
    goSetLowerLimit.valueNoSend(lowerLimit);
    upperLimit = stateReader.readWord();
    if (upperLimit == 0xFFFF)
    {
        upperLimit = 1023;
        goSetUpperLimit.requestObjectRead();
    }
    goSetUpperLimit.valueNoSend(upperLimit);
}

uint32_t ForceSensor::getLowerLimit()
{
    return lowerLimit;
}

uint32_t ForceSensor::getUpperLimit()
{
    return upperLimit;
}

uint32_t ForceSensor::getRaw()
{
    return 1023 - analogRead(pin);
}

ForceSensorSum::ForceSensorSum(const char *name, int &groupObjectIndex, uint32_t &parameterAddress, GroupObjectUpdatedHandler callback, ForceSensor **sensors, size_t sensorCount)
    : ForceSensorBase(name, groupObjectIndex, parameterAddress, callback),
      sensors(sensors),
      sensorCount(sensorCount),
      goDetectedAny(knx.getGroupObject(groupObjectIndex++)),
      goLockDetectedAny(knx.getGroupObject(groupObjectIndex++)),
      goSwitchDelayedOffAny(knx.getGroupObject(groupObjectIndex++)),
      lockModeAnyDetected((LockMode)readKnxParameterUInt8(name, "LockModeDetected", parameterAddress)),
      delayedOffTimeAny(getDelayedOffTime((DelayedSwitchOffTime)readKnxParameterUInt8(name, "SwitchOffTimeAny", parameterAddress)))

{
    goDetectedAny.dataPointType(DPT_Switch);
    goLockDetectedAny.callback(callback);
    goLockDetectedAny.dataPointType(DPT_Enable);
    goSwitchDelayedOffAny.dataPointType(DPT_Switch);
    goSwitchDelayedOffAny.callback(callback);
}

void ForceSensorSum::callback(GroupObject &groupObject)
{
    ForceSensorBase::callback(groupObject);
    if (goLockDetectedAny.asap() == groupObject.asap())
    {
        lockAnyDetected = goLockDetectedAny.value();
        logValue(name, "Manual control Detected Any received", lockAnyDetected);
        StateWriter::RequestSave();
    }
}
void ForceSensorSum::writeState(StateWriter &stateWriter)
{
    ForceSensorBase::writeState(stateWriter);
    stateWriter.writeByte(lockAnyDetected ? 1 : 0);
}

void ForceSensorSum::readState(StateReader &stateReader)
{
    ForceSensorBase::readState(stateReader);
    uint8_t lockAnyDetectedRaw = stateReader.readByte();
    if (lockAnyDetectedRaw == 0xFF)
    {
        lockAnyDetected = false;
        goLockDetectedAny.requestObjectRead();
    }
    else if (lockAnyDetected == 1)
        lockAnyDetected = true;
    else
        lockAnyDetected = false;
    goLockDetectedAny.valueNoSend(lockAnyDetected);
}

void ForceSensorSum::loop(unsigned long now, bool diagnosticMode, bool forceSent)
{
    ForceSensorBase::loop(now, diagnosticMode, forceSent);
    bool anyDetected = false;
    if (lockAnyDetected)
    {
        bool anyDetected = getLastDetected();
        if (!anyDetected)
        {
            for (size_t i = 0; i < sensorCount; i++)
            {
                if (sensors[i]->getLastDetected())
                {
                    anyDetected = true;
                    break;
                }
            }
        }
    }
    else
    {
        switch (lockModeAnyDetected)
        {
            case LockMode::LockCurrentState:
                anyDetected = lastAnyDetected;
                break;
            case LockMode::SwitchToOff:
                anyDetected = false;
                break;
            case LockMode::SwitchToOn:
                anyDetected = true;
                break;
        }
    }
    if (lastAnyDetected != anyDetected || forceSent)
    {
        lastAnyDetected = anyDetected;
        logValue(name, "Send DetectedAny", lastAnyDetected);
        goDetectedAny.value(anyDetected);
        delayedOffTimerAny = now;
    }
    HandleDelayedOff("Send SwitchDelayedOffAny", now, forceSent, delayedOffTimeAny, delayedOffTimerAny, lastSwitchDelayedAny, anyDetected, goSwitchDelayedOffAny);
}

uint32_t ForceSensorSum::getLowerLimit()
{
    uint32_t lowerLimit = 0;
    for (size_t i = 0; i < sensorCount; i++)
    {
        lowerLimit += sensors[i]->getLowerLimit();
    }
    return lowerLimit;
}
uint32_t ForceSensorSum::getUpperLimit()
{
    uint32_t upperLimit = 0;
    for (size_t i = 0; i < sensorCount; i++)
    {
        upperLimit += sensors[i]->getUpperLimit();
    }
    return upperLimit;
}
uint32_t ForceSensorSum::getRaw()
{
    uint32_t raw = 0;
    for (size_t i = 0; i < sensorCount; i++)
    {
        raw += sensors[i]->getRaw();
    }
    return raw;
}