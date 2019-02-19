#ifndef M3_TYPES_H__
#define M3_TYPES_H__

#include <Arduino.h>

namespace m3
{

typedef uint8_t byte;
typedef uint16_t NodeId;
typedef uint16_t PackageSize;
typedef uint64_t PackageNumber;
typedef uint8_t StateSize;
typedef uint16_t KeySize;
typedef uint16_t HmacSize;

enum NodeType : byte
{
    Sensor = 1,
    Actor = 2,
    Controller = 3
};

} // namespace m3

#endif