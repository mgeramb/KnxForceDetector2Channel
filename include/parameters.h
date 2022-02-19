#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <Arduino.h>

uint8_t readKnxParameterUInt8(const char* name, const char* operation, uint32_t &parameterAddress);

uint32_t readKnxParameterUInt32(const char* name, const char* operation, uint32_t &parameterAddress);

#endif