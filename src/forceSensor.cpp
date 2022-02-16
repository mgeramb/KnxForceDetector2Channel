#include <knx.h>
#include "forceSensor.h"
#include "log.h"
#include "parameters.h"

ForceSensorBase::ForceSensorBase(const char *name, int &groupObjectIndex, uint32_t& parameterAddress, GroupObjectUpdatedHandler callback)
    : name(name),
      goErrorForce(knx.getGroupObject(groupObjectIndex++)),
      goForce(knx.getGroupObject(groupObjectIndex++)),
      goForcePercentage(knx.getGroupObject(groupObjectIndex++)),
      goForceDetected(knx.getGroupObject(groupObjectIndex++)),
      goManualControlForceDetected(knx.getGroupObject(groupObjectIndex++)),
      goSetDetectionLimit(knx.getGroupObject(groupObjectIndex++)),
      hysterese(readKnxParameterUInt32(name, "Hysterese", parameterAddress))
{
    Serial.println("initGroupObjects base");
    goErrorForce.dataPointType(DPT_Switch);
    goForce.dataPointType(DPT_Value_2_Count);
    goForcePercentage.dataPointType(DPT_Scaling);
    goForceDetected.dataPointType(DPT_Switch);
    goManualControlForceDetected.callback(callback);
    goManualControlForceDetected.dataPointType(DPT_Switch);
    goSetDetectionLimit.dataPointType(DPT_Scaling);
}

void ForceSensorBase::callback(GroupObject& groupObject)
{
    if (&goSetDetectionLimit == &groupObject)
    {
        detectionLimit = goSetDetectionLimit.value();
        logValue(name, "Detection limit received", detectionLimit);
    }
     if (&goManualControlForceDetected == &groupObject)
    {
        manualControlForceDetected = (ManualControl)  (uint8_t) goManualControlForceDetected.value();
        logValue(name, "Detection limit received", detectionLimit);
    }
}

void ForceSensorBase::writeState(StateWriter& stateWriter)
{
    stateWriter.writeByte(detectionLimit);
    stateWriter.writeByte(manualControlForceDetected);
}

void ForceSensorBase::readState(StateReader& stateReader)
{
    detectionLimit = stateReader.readByte();
    if (detectionLimit == 0xFF)
    {
        detectionLimit = 50;
        goSetDetectionLimit.requestObjectRead();
    }
    goSetDetectionLimit.valueNoSend(detectionLimit);
    manualControlForceDetected = (ManualControl)stateReader.readByte();
    if (manualControlForceDetected == 0xFF)
    {
        manualControlForceDetected = ManualControl::ForceOff;
        goManualControlForceDetected.requestObjectRead();
    }
    goManualControlForceDetected.valueNoSend((byte) manualControlForceDetected);
}

void ForceSensorBase::loop(unsigned long now, bool diagnosticMode, bool forceSent)
{
    uint32_t raw = getRaw();
    uint32_t lowerLimit = getLowerLimit();
    uint32_t upperLimit = getUpperLimit();
    bool detected = false;
    bool error = false;
    byte percent = 0;
    if (raw < lowerLimit)
    {
        error = true;
        detected = true;
        percent = 0;
    }
    else if (raw > upperLimit)
    {
        error = true;
        detected = true;
        percent = 100;
    }
    else
    {
        float onePercent = ((float)(upperLimit - lowerLimit)) / (float)100;
        percent = (byte)((float)(raw - lowerLimit) / onePercent);
        byte currentDetectionLimit = detectionLimit;
        if (lastDetected)
        {
            currentDetectionLimit =- hysterese;
            if (currentDetectionLimit < 1)
                currentDetectionLimit = 1;
        }
        if (percent >= currentDetectionLimit)
            detected = true;
    }
    
    if (manualControlForceDetected == ManualControl::ForceOn)
        detected = true;
    if (manualControlForceDetected == ManualControl::ForceOff)
        detected = false;
    if (!diagnosticMode)
        raw = 0;
    if (lastRaw != raw || forceSent)
    {
        lastRaw = raw;
        logValue(name, "Send force", lastRaw);
        goForce.value(lastRaw);
    }
    if (lastPercent != percent || forceSent)
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
    if (lastDetected != detected || forceSent)
    {
        lastDetected = detected;
        logValue(name, "Send force detected", lastDetected);
        goForceDetected.value(detected);
    }
}

bool ForceSensorBase::getLastDetected()
{
    return lastDetected;
}

ForceSensor::ForceSensor(uint32_t pin, const char *name, int& groupObjectIndex, uint32_t& parameterAddress, GroupObjectUpdatedHandler callback) : 
    ForceSensorBase(name, groupObjectIndex, parameterAddress, callback), 
    pin(pin),
    goSetLowerLimit(knx.getGroupObject(groupObjectIndex++)),
    goSetUpperLimit(knx.getGroupObject(groupObjectIndex++))
{
    Serial.println("initGroupObjects");
    goSetLowerLimit.callback(callback);
    goSetLowerLimit.dataPointType(DPT_Value_2_Count);
    goSetUpperLimit.callback(callback);
    goSetUpperLimit.dataPointType(DPT_Value_2_Count);
}

void ForceSensor::callback(GroupObject& groupObject)
{
    ForceSensorBase::callback(groupObject);
    if (&goSetLowerLimit == &groupObject)
    {
        lowerLimit = goSetLowerLimit.value();
        logValue(name, "Lower limit received", detectionLimit);
    }
    else if (&goSetUpperLimit == &groupObject)
    {
        upperLimit = goSetLowerLimit.value();
        logValue(name, "Upper limit received", detectionLimit);
    }
}

void ForceSensor::writeState(StateWriter& stateWriter)
{
    ForceSensorBase::writeState(stateWriter);
    stateWriter.writeWord(lowerLimit);
    stateWriter.writeWord(upperLimit);
}

void ForceSensor::readState(StateReader& stateReader)
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

ForceSensorSum::ForceSensorSum(const char* name, int& groupObjectIndex, uint32_t& parameterAddress, GroupObjectUpdatedHandler callback, ForceSensor** sensors, size_t sensorCount)
    : ForceSensorBase(name, groupObjectIndex, parameterAddress, callback),
    sensors(sensors),
    sensorCount(sensorCount),
    goDetectedAny(knx.getGroupObject(groupObjectIndex))
{

}

void ForceSensorSum::loop(unsigned long now, bool diagnosticMode, bool forceSent)
{
    ForceSensorBase::loop(now, diagnosticMode, forceSent);
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
    if (lastAnyDetected != anyDetected || forceSent)
    {
        lastAnyDetected = anyDetected;
        logValue(name, "Send DetectedAny", lastAnyDetected);
        goDetectedAny.value(lastRaw);
    }
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