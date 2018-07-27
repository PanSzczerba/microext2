#include "crc.h"

#define BITS_IN_BYTE 8
#define BYTE_END_BIT 0x80
#define SIGNIFICANT_BIT_MASK 0x1

uint32_t mext2_generic_crc(uint8_t* data, size_t data_length, uint32_t polynomial, uint8_t polynomial_length)
{
    uint32_t result = 0;

    for(size_t i = 0; i < data_length; i++)
    {
        uint8_t octet = data[i];
        for(uint8_t j = 0; j < BITS_IN_BYTE; j++)
        {
            if(((result >> (polynomial_length - 1)) ^ (octet >> (BITS_IN_BYTE - 1))) & SIGNIFICANT_BIT_MASK)
                result = ((result << 1) ^ polynomial);
            else
                result <<= 1;
            octet <<= 1;
        }
    }

    for(uint8_t i = 0; i < sizeof(uint32_t) * BITS_IN_BYTE - polynomial_length; i++)
        result &= ~(1 << (sizeof(uint32_t) * BITS_IN_BYTE - 1 - i));

    return result;
}
