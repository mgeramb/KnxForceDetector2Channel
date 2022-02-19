#include "states.h"
#include "log.h"

unsigned long StateWriter::SaveRequestedAt = 0;

void StateWriter::RequestSave()
{
    if (SaveRequestedAt == 0)
    {
        SaveRequestedAt = millis();
        if (SaveRequestedAt == 0)
            SaveRequestedAt = 1;
        logValue("SaveWriter", "Request Save At", SaveRequestedAt);
    }
}
bool StateWriter::CheckSaveNeededAndResetRequest(long now)
{
    if (SaveRequestedAt != 0 && now - SaveRequestedAt > 10000)
    {
        SaveRequestedAt = 0;
        logValue("SaveWriter", "Save At", now);
        return true;
    }
    return false;
}

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
    *buffer = (uint8_t) (value >> 8);
    buffer += sizeof(uint8_t);
    *buffer = (uint8_t) (value & 0xFF);
    buffer += sizeof(uint8_t);
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
    Serial.println(result);
    buffer += sizeof(uint8_t);
    return result;
}

uint16_t StateReader::readWord()
{
    uint16_t result = *buffer;
    buffer += sizeof(uint8_t);
    result = result << 8;
    result |= (uint16_t) *buffer;
    buffer += sizeof(uint8_t);
    Serial.println(result);
    return result;
}

const uint8_t* StateReader::currentBuffer()
{
    return buffer;
}
