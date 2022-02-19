#include "parameters.h"
#include "log.h"
#include "knx.h"

uint8_t readKnxParameterUInt8(const char* name, const char* operation, uint32_t &parameterAddress)
{
    if (ArduinoPlatform::SerialDebug != NULL)
    {
        ArduinoPlatform::SerialDebug->print("KNX Parameter Offset ");
        ArduinoPlatform::SerialDebug->print(parameterAddress);
        ArduinoPlatform::SerialDebug->print(": ");
    }
    uint8_t result = knx.paramByte(parameterAddress);
    parameterAddress += 1;
    logValue(name, operation, result);
    return result;
}

uint32_t readKnxParameterUInt32(const char* name, const char* operation, uint32_t &parameterAddress)
{
    if (ArduinoPlatform::SerialDebug != NULL)
    {
        ArduinoPlatform::SerialDebug->print("KNX Parameter Offset ");
        ArduinoPlatform::SerialDebug->print(parameterAddress);
        ArduinoPlatform::SerialDebug->print(": ");
    }
    uint32_t result = knx.paramInt(parameterAddress);
    parameterAddress += 4;
    logValue(name, operation, result);
    return result;
}

