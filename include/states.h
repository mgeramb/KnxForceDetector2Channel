#ifndef STATES_H
#define STATES_H

#include <Arduino.h>


class StateWriter
{
    static unsigned long SaveRequestedAt;
public:
    static void RequestSave();
    static bool CheckSaveNeededAndResetRequest(long now);

private:
    uint8_t *buffer;
public:
    StateWriter(uint8_t* buffer);
    void writeByte(uint8_t value);
    void writeWord(uint16_t value);
    uint8_t* currentBuffer();
};

class StateReader
{
    const uint8_t* buffer;
 public:
     StateReader(const uint8_t* buffer);
     uint8_t readByte();
     uint16_t readWord();
     const uint8_t* currentBuffer();
};

#endif