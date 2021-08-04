#include "time.h"
#include <Arduino.h>

bool isTimeoutLeft(uint16_t timeOut, uint64_t *prevTime)
{
    uint64_t currentTime = millis();

    if (currentTime - *prevTime > timeOut)
    {
        *prevTime = currentTime;
        return true;
    }
    else
    {
        return false;
    }
}