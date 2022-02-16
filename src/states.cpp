#include "states.h"

StateWriter::StateWriter(uint8_t* buffer) :
    buffer(buffer)
{
} 

void StateWriter::writeByte(uint8_t value)
{
    *buffer = value;
    buffer += sizeof(uint8_t);
}

void StateWriter::writeWord(uint16_t value)
{
    *(uint16_t *) buffer = value;
    buffer += sizeof(uint16_t);
}

uint8_t* StateWriter::currentBuffer()
{
    return buffer;
}

StateReader::StateReader(const uint8_t* buffer) :
    buffer(buffer)
{
} 

uint8_t StateReader::readByte()
{
    uint8_t result = *buffer;
    buffer += sizeof(uint8_t);
    return result;
}

uint16_t StateReader::readWord()
{
    uint16_t result = *(const uint16_t *)buffer;
    buffer += sizeof(uint16_t);
    return result;
}

const uint8_t* StateReader::currentBuffer()
{
    return buffer;
}
