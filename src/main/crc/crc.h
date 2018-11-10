#ifndef MEXT2_CRC_H
#define MEXT2_CRC_H
#include <stdint.h>
#include <stddef.h>

#define CRC7_POLYNOMIAL 0x09
#define CRC16_POLYNOMIAL 0x1021
#define CRC32_POLYNOMIAL 0x04c11db7

#define crc7(data, data_length) mext2_generic_crc((data), (data_length), CRC7_POLYNOMIAL, 7)
#define crc16(data, data_length) mext2_generic_crc((data), (data_length), CRC16_POLYNOMIAL, 16)
#define crc32(data, data_length) mext2_generic_crc((data), (data_length), CRC32_POLYNOMIAL, 32)

uint32_t mext2_generic_crc(uint8_t* data, size_t data_length, uint32_t polynomial, uint8_t polynomial_length);
#endif
