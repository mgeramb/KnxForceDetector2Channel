#include <knx.h>
#include "forceSensor.h"
#include "log.h"

ForceSensorBase::ForceSensorBase(const char *name)
    : name(name)
{
    
}

void ForceSensorBase::initGroupObjects(int& groupObjectIndex, GroupObjectUpdatedHandler callback)
{
    Serial.println("initGroupObjects base");
    goErrorForce = knx.getGroupObject(1 /*groupObjectIndex++*/);
    goForce = knx.getGroupObject(1 /*groupObjectIndex++*/);
    goForcePercentage = knx.getGroupObject(1 /*groupObjectIndex++*/);
    goForceDetected = knx.getGroupObject(1 /*groupObjectIndex++*/);
    goManualControlForceDetected = knx.getGroupObject(1 /*groupObjectIndex++*/);
    goSetDetectionLimit = knx.getGroupObject(1 /*groupObjectIndex++*/);
    goErrorForce.dataPointType(DPT_Switch);
    goForce.dataPointType(DPT_Value_2_Count);
    goForcePercentage.dataPointType(DPT_Percent_U8);
    goForceDetected.dataPointType(DPT_Switch);
    //goManualControlForceDetected.callback(callback);
    goManualControlForceDetected.dataPointType(DPT_Switch);
    goSetDetectionLimit.dataPointType(DPT_Percent_U8);
}

void ForceSensorBase::callback(GroupObject& groupObject)
{
    if (&goSetDetectionLimit == &groupObject)
    {
        detectionLimit = goSetDetectionLimit.value();
        log(name, "Detection limit received", detectionLimit);
    }
}

ForceSensor::ForceSensor(uint32_t pin, const char *name) : 
    ForceSensorBase(name), 
    pin(pin)
{
   
}

void ForceSensor::initGroupObjects(int& groupObjectIndex, GroupObjectUpdatedHandler callback)
{
    Serial.println("initGroupObjects");
    ForceSensorBase::initGroupObjects(groupObjectIndex, callback);
    goSetLowerLimit = knx.getGroupObject(1 /*groupObjectIndex++*/);
    goSetUpperLimit = knx.getGroupObject(1 /*groupObjectIndex++*/);
   // goSetLowerLimit.callback(callback);
    goSetLowerLimit.dataPointType(DPT_Value_2_Count);
    //goSetUpperLimit.callback(callback);
    goSetUpperLimit.dataPointType(DPT_Value_2_Count);
}

void ForceSensor::callback(GroupObject& groupObject)
{
    ForceSensorBase::callback(groupObject);
    if (&goSetLowerLimit == &groupObject)
    {
        lowerLimit = goSetLowerLimit.value();
        log(name, "Lower limit received", detectionLimit);
    }
    else if (&goSetUpperLimit == &groupObject)
    {
        upperLimit = goSetLowerLimit.value();
        log(name, "Upper limit received", detectionLimit);
    }
}

void ForceSensor::loop(unsigned long now, bool diagnosticMode, bool forceSent)
{
    uint32_t raw = 1023 - analogRead(pin);
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
        if (percent >= detectionLimit)
            detected = true;
    }
    
    if (manualControlForceDetected)
        detected = true;
    if (!diagnosticMode)
        raw = 0;
    if (lastRaw != raw || forceSent)
    {
        lastRaw = raw;
        log(name, "Send force", lastRaw);
        //goForce.value(lastRaw);
    }
    if (lastPercent != percent || forceSent)
    {
        lastPercent = percent;
        log(name, "Send percent", lastPercent);
        //goForcePercentage.value(lastPercent);
    }
    if (lastError != error || forceSent)
    {
        lastError = error;
        log(name, "Send error", lastError);
        //goErrorForce.value(lastError);
    }
    if (lastDetected != detected || forceSent)
    {
        lastDetected = detected;
        log(name, "Send force detected", lastDetected);
        //goForceDetected.value(detected);
    }
}