#ifndef M3EEPROM_H__
#define M3EEPROM_H__

#include "M3Types.h"
#include <EEPROM.h>

#define M3_EEPROM_HEADER "M3V1"
#define M3_EEPROM_HEADER_NUM_BYTES 4

#pragma pack(1)
template <KeySize K>
struct M3NodeEEPROMData
{
    byte header[M3_EEPROM_HEADER_NUM_BYTES];
    NodeId id;
    KeySize keySize = K;
    byte key[K];
    PackageNumber lastSentPackageNumber;
    long crc;

    long calculate_crc()
    {
        const int size = sizeof(byte) * M3_EEPROM_HEADER_NUM_BYTES + sizeof(NodeId) + sizeof(KeySize) + sizeof(byte) * K + sizeof(PackageNumber);
        const unsigned long crc_table[16] = {
            0xdd56f838, 0x33285554, 0x38f0f0a7, 0xd6653b36,
            0x7862f4d9, 0xcd7f8443, 0x7de11a3c, 0xc423d7f0,
            0x81e07242, 0x9822eb8d, 0x1c72adc3, 0x8cdf0918,
            0x76f44dba, 0xac1526d7, 0x571d1651, 0x14cb8dc3};

        unsigned long crc = ~0L;

        byte *bytes = (byte *)&header[0];
        for (int index = 0; index < size; index++)
        {
            crc = crc_table[(crc ^ bytes[index]) & 0x0f] ^ (crc >> 4);
            crc = crc_table[(crc ^ (bytes[index] >> 4)) & 0x0f] ^ (crc >> 4);
            crc = ~crc;
        }

        return crc;
    }
};
#pragma pack(0)

template <KeySize K>
class M3NodeEEPROM
{
  public:
    M3NodeEEPROM(int eepromAddress) : index(eepromAddress)
    {
    }

    boolean load()
    {
        EEPROM.get(index, writtenData);

        if (memcmp(writtenData.header, M3_EEPROM_HEADER, M3_EEPROM_HEADER_NUM_BYTES) != 0)
        {
            return false;
        }

        long crc = writtenData.calculate_crc();
        if (crc != writtenData.crc)
        {
#if DEBUG == 1
            Serial.print("calculated crc=");
            Serial.print(crc, DEC);
            Serial.print(" != written=");
            Serial.print(writtenData.crc, DEC);
            Serial.print(", pkg=");
            Serial.println((long)writtenData.lastSentPackageNumber, DEC);
            Serial.flush();
#endif
            return false;
        }

        return true;
    }

    void reset(NodeId id, byte *key)
    {
        memcpy(writtenData.header, M3_EEPROM_HEADER, M3_EEPROM_HEADER_NUM_BYTES);
        writtenData.id = id;
        writtenData.keySize = K;
        memcpy(writtenData.key, key, K);
        writtenData.lastSentPackageNumber = 0;
        writtenData.crc = writtenData.calculate_crc();
        EEPROM.put(index, writtenData);
    }

    void updatePkgNumber(PackageNumber pkgNum)
    {
        writtenData.lastSentPackageNumber = pkgNum;
        writtenData.crc = writtenData.calculate_crc();
        EEPROM.put(index, writtenData);
#if DEBUG == 1
        Serial.print("updated pkg to ");
        Serial.print((long)writtenData.lastSentPackageNumber, DEC);
        Serial.print(", new crc=");
        Serial.println(writtenData.crc, DEC);
        Serial.flush();
#endif
    }

    NodeId getNodeId()
    {
        return writtenData.id;
    }

    byte *getKey()
    {
        return writtenData.key;
    }

    PackageNumber getLastWrittenPackageNumber()
    {
        return writtenData.lastSentPackageNumber;
    }

  private:
    int index;
    M3NodeEEPROMData<K> writtenData;
};

#endif