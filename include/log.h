#ifndef LOG_H
#define LOG_H

#include <knx.h>

void log(const char* name, const char* operation, float value)
{
    if (ArduinoPlatform::SerialDebug != NULL)
    {
        ArduinoPlatform::SerialDebug->print(name);
        ArduinoPlatform::SerialDebug->print(" ");
        ArduinoPlatform::SerialDebug->print(operation);
        ArduinoPlatform::SerialDebug->print(" ");
        ArduinoPlatform::SerialDebug->println(value);
    }
}

#endif