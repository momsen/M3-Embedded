#include "blocking.h"

namespace utils
{

#define WASTE_CYCLE __asm__("nop\n\t");

byte blockingReadDebouncedValueLoop(byte pin, long loopCount, long wasteCount)
{
    long restDebounceCount = loopCount;
    byte readValue = digitalRead(pin);

    while (restDebounceCount-- > 0)
    {
        byte currentValue = digitalRead(pin);
        if (currentValue != readValue)
        {
            restDebounceCount = loopCount;
            readValue = currentValue;
        }

        for (long i = 0; i < wasteCount; i++)
        {
            WASTE_CYCLE
        }
    }

    return readValue;
}

long blockingMeasureStateTime(byte pin, long maxTime, byte expectedValue)
{
    long startTime = millis();
    while (true)
    {
        long time = millis() - startTime;
        if (time >= maxTime || digitalRead(pin) != expectedValue)
        {
            return time;
        }
    }
}

void blockingFlashLed(byte pin, long times, long delayTime, byte stateAfterwards)
{
    for (int i = 0; i < times; i++)
    {
        digitalWrite(pin, HIGH);
        delay(delayTime);
        digitalWrite(pin, LOW);
        delay(delayTime);
    }
    digitalWrite(pin, stateAfterwards);
}

} // namespace utils