#include "parameters.h"
#include "log.h"
#include "knx.h"

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

