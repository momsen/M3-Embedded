#ifndef BLOCKING_UTILS_H__
#define BLOCKING_UTILS_H__

#include "Arduino.h"

namespace utils
{

byte blockingReadDebouncedValueLoop(byte pin, long loopCount, long wasteCount);

long blockingMeasureStateTime(byte pin, long maxTime, byte expectedValue);

void blockingFlashLed(byte pin, long times, long delayTime, byte stateAfterwards);

} // namespace utils

#endif