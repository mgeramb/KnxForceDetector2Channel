#include <knx.h>

void logValue(const char* name, const char* operation, float value)
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