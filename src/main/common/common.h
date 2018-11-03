#ifndef MEXT2_COMMON_H
#define MEXT2_COMMON_H

#include <stdint.h>

#define STATIC static

#define __mext2_packed __attribute__((packed))

// Boolean values
#define MEXT2_TRUE 1
#define MEXT2_FALSE 0

typedef enum mext2_return_value
{
    MEXT2_RETURN_FAILURE = 0,
    MEXT2_RETURN_SUCCESS
} mext2_return_value;

uint8_t mext2_is_big_endian(void);

uint16_t mext2_flip_endianess16(uint16_t num);
uint32_t mext2_flip_endianess32(uint32_t num);
uint64_t mext2_flip_endianess64(uint64_t num);

#define mext2_le_to_cpu16(num) (mext2_is_big_endian() ? (num) : mext2_flip_endianess16(num))
#define mext2_le_to_cpu32(num) (mext2_is_big_endian() ? (num) : mext2_flip_endianess32(num))
#define mext2_le_to_cpu64(num) (mext2_is_big_endian() ? (num) : mext2_flip_endianess64(num))

#define mext2_cpu_to_le16(num) (mext2_is_big_endian() ? (num) : mext2_flip_endianess16(num))
#define mext2_cpu_to_le32(num) (mext2_is_big_endian() ? (num) : mext2_flip_endianess32(num))
#define mext2_cpu_to_le64(num) (mext2_is_big_endian() ? (num) : mext2_flip_endianess64(num))

#endif
