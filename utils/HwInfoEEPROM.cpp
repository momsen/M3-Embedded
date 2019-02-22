#include <HwInfoEEPROM.h>

namespace utils
{
uint32_t HwInfoEEPROMData::calculate_crc()
{
    const uint16_t size = sizeof(uint8_t) * HWINFO_EEPROM_HEADER_NUM_BYTES + sizeof(uint16_t);

    const uint32_t crc_table[16] = {
        0xdd56f838, 0x33285554, 0x38f0f0a7, 0xd6653b36,
        0x7862f4d9, 0xcd7f8443, 0x7de11a3c, 0xc423d7f0,
        0x81e07242, 0x9822eb8d, 0x1c72adc3, 0x8cdf0918,
        0x76f44dba, 0xac1526d7, 0x571d1651, 0x14cb8dc3};

    uint32_t crc = ~0L;
    uint8_t *bytes = (uint8_t *)&header[0];
    for (uint16_t index = 0; index < size; index++)
    {
        crc = crc_table[(crc ^ bytes[index]) & 0x0f] ^ (crc >> 4);
        crc = crc_table[(crc ^ (bytes[index] >> 4)) & 0x0f] ^ (crc >> 4);
        crc = ~crc;
    }

    return crc;
}

HwInfoEEPROM::HwInfoEEPROM(uint16_t address) : address(address)
{
}

boolean HwInfoEEPROM::load()
{
    EEPROM.get(address, writtenData);

    if (memcmp(writtenData.header, HWINFO_EEPROM_HEADER, HWINFO_EEPROM_HEADER_NUM_BYTES) != 0)
    {
        return false;
    }

    uint32_t crc = writtenData.calculate_crc();
    if (crc != writtenData.crc)
    {
        return false;
    }

    return true;
}

uint16_t HwInfoEEPROM::getVcc()
{
    return writtenData.vcc;
}

} // namespace utils