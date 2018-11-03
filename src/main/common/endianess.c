#include "common.h"

#define BITS_IN_BYTE 8

uint8_t mext2_is_big_endian(void)
{
    union
    {
        uint32_t u32;
        uint8_t arr[4];
    } dummy = { 0x01020304 };

    return dummy.arr[0] == 0x1;
}

uint16_t mext2_flip_endianess16(uint16_t num)
{
    uint8_t temp = (uint8_t)num;
    num >>= BITS_IN_BYTE;
    num |= (((uint16_t)temp) << BITS_IN_BYTE);

    return num;
}

uint32_t mext2_flip_endianess32(uint32_t num)
{
    union
    {
        uint32_t u32;
        uint8_t arr[4];
    } inv_num;

    inv_num.u32 = num;

    uint8_t temp = inv_num.arr[0];
    inv_num.arr[0] = inv_num.arr[3];
    inv_num.arr[3] = temp;

    temp = inv_num.arr[1];
    inv_num.arr[1] = inv_num.arr[2];
    inv_num.arr[2] = temp;

    return inv_num.u32;
}

uint64_t mext2_flip_endianess64(uint64_t num)
{

    union
    {
        uint64_t u64;
        uint8_t arr[8];
    } inv_num;

    inv_num.u64 = num;

    uint8_t temp = inv_num.arr[0];
    inv_num.arr[0] = inv_num.arr[7];
    inv_num.arr[7] = temp;

    temp = inv_num.arr[1];
    inv_num.arr[1] = inv_num.arr[6];
    inv_num.arr[6] = temp;

    temp = inv_num.arr[2];
    inv_num.arr[2] = inv_num.arr[5];
    inv_num.arr[5] = temp;

    temp = inv_num.arr[3];
    inv_num.arr[3] = inv_num.arr[4];
    inv_num.arr[4] = temp;

    return inv_num.u64;
}
