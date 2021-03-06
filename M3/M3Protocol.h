#ifndef M3_PROTOCOL_H__
#define M3_PROTOCOL_H__

#include "M3Types.h"

namespace m3
{

#pragma pack(1)

enum PackageType : char
{
    Identify = 'I',
    Data = 'D',
    Command = 'C',
    Replay = 'R'
};

#define M3_PACKAGE_START "M3v1"
#define M3_PACKAGE_START_NUM_BYTES 4
#define M3_PACKAGE_END ";"
#define M3_PACKAGE_END_NUM_BYTES 1

template <StateSize S, KeySize K, HmacSize H>
struct IdentifyPackage
{
    byte start[M3_PACKAGE_START_NUM_BYTES];
    PackageType pkgType = Identify;
    PackageSize size = sizeof(IdentifyPackage);
    NodeId id;
    NodeType nodeType;
    KeySize keySize = K;
    HmacSize hmacSize = H;
    StateSize stateSize = S;
    short batteryVoltage;
    byte batteryLevel;
    byte key[K];
    byte end[M3_PACKAGE_END_NUM_BYTES];

    void initialize(NodeId id, NodeType type, byte *key)
    {
        memcpy(start, M3_PACKAGE_START, M3_PACKAGE_START_NUM_BYTES);
        memcpy(end, M3_PACKAGE_END, M3_PACKAGE_END_NUM_BYTES);
        memcpy(&(this->key[0]), key, K);

        this->id = id;
        this->nodeType = type;
    }
};

template <StateSize S, HmacSize H>
struct DataPackage
{
    byte start[M3_PACKAGE_START_NUM_BYTES];
    byte pkgType = Data;
    PackageSize size = sizeof(DataPackage);
    NodeId id;
    PackageNumber pkgId;
    short batteryVoltage;
    byte batteryLevel;
    byte state[S];
    byte hmac[H];
    byte end[M3_PACKAGE_END_NUM_BYTES];

    void initialize(NodeId id, PackageNumber pkgId)
    {
        memcpy(start, M3_PACKAGE_START, M3_PACKAGE_START_NUM_BYTES);
        memcpy(end, M3_PACKAGE_END, M3_PACKAGE_END_NUM_BYTES);

        this->id = id;
        this->pkgId = pkgId;
    }
};

#pragma pack(0)

} // namespace m3

#endif