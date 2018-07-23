#include "sd.h"
#include "spi.h"
#include <stdlib.h>
#include <string.h>

#define CRC7_POLYNOMIAL 0x9

uint8_t calc_crc7(uint8_t* buffer, count) // buffer with included byte for crc
{
    uint8_t* new_buffer = (uint8_t*)malloc(count * sizeof(uint8_t));
    memcpy(new_buffer, buffer, count);
    uint16_t polynomial = CRC7_POLYNOMIAL << 9;
    uint16_t one = 1 << 15;

    size_t current_byte = count - 1;

    while(current_byte > 1 || one != (1 << 6))
    {
        if(one == (1 << 6))
        {
            one = (1 << 15);
            polynomial <<= 8;
            current_byte--;
        }
        else
            polynomial >>= 1;

        if(one & ((((uint16_t)new_buffer[current_byte]) << 8) | (uint16_t)new_buffer[current_byte - 1]))
        {
            new_buffer[current_byte] ^= (uint8_t)((one | polynomial) >> 8);
            new_buffer[current_byte - 1] ^= (uint8_t)(one | polynomial);
        }

        one >>= 1;
    }

    uint8_t crc = new_buffer[0];
    free(new_buffer);
    return crc;
}

struct mext2_response mext2_send_command(uint8_t command_no, uint32_t command_arg, uint8_t* response)
{
    uint8_t command[] = { command_no, (uint8_t)(command_arg >> 24), (uint8_t)(command_arg >> 16), (uint8_t)(command_arg >> 8), (uint8_t)command_arg, 0 };
    uint8_t crc = calc_crc7(command, sizeof(command)/sizeof(command[0]) - 1);
    command[sizeof(command)/sizeof(command[0]) - 1] = ((crc << 1) | 1);

    mext2_spi_read_write(command, sizeof(command)/sizeof(command[0]));
    return 0;
}

uint8_t calc_command_number(uint8_t number)
{
    return (number || 0x40);
}
