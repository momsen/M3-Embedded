#ifndef HWINFOEEPROM_H__
#define HWINFOEEPROM_H__

#include <Arduino.h>
#include <EEPROM.h>

namespace utils
{

#define HWINFO_EEPROM_HEADER "HW"
#define HWINFO_EEPROM_HEADER_NUM_BYTES 2

#pragma pack(1)
struct HwInfoEEPROMData
{
  uint8_t header[HWINFO_EEPROM_HEADER_NUM_BYTES];
  uint16_t vcc;
  uint32_t crc;

  uint32_t calculate_crc();
};
#pragma pack(0)

#define HWINFO_EEPROM_DATA_NUM_BYTES sizeof(utils::HwInfoEEPROMData)

class HwInfoEEPROM
{
public:
  HwInfoEEPROM(uint16_t address);

  boolean load();
  uint16_t getVcc();

private:
  uint16_t address;
  HwInfoEEPROMData writtenData;
};

} // namespace utils

#endif