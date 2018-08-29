#include "sd.h"
#include "spi.h"

#include "debug.h"
#include "pin.h"

#include <stdlib.h>
#include <string.h>

#define CRC7_POLYNOMIAL 0x9

uint8_t calc_crc7(uint8_t* buffer, uint8_t count) // buffer with included byte for crc
{
    uint8_t* new_buffer = (uint8_t*)malloc(count * sizeof(uint8_t));
    memcpy(new_buffer, buffer, count);
    uint16_t polynomial = CRC7_POLYNOMIAL << 9;
    uint16_t one = 1 << 15;

    uint8_t current_byte = count - 1;

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
    crc = (crc << 1) | 1;
    free(new_buffer);
    return crc;
}

mext2_response* send_command(mext2_command* command, mext2_response_type response_type)
{
    uint8_t* c = (uint8_t*)command;
    spi_read_write(c, COMMAND_SIZE);

    uint8_t buffer[] = {0xff, 0xff, 0xff, 0xff, 0xff};

    uint16_t i;
    for(i = 0; i < N_CYCLES_TIMEOUT && buffer[0] == 0xff; i++)
    {
        buffer[0] = 0xff;
        spi_read_write(buffer, 1);
    }

    if(i == N_CYCLES_TIMEOUT)
    {
        debug("Error: exceeded time limit waiting for response, check your SD card reader device.\n");
        return NULL;
    }
    
    if(response_type == MEXT2_R7 || response_type == MEXT2_R3)
    {
        spi_read_write(&buffer[1], 4);
    }
    mext2_response* response = (mext2_response*)buffer;

    return response;
}

uint8_t calc_command_number(uint8_t number)
{
    return (number || 0x40);
}

void wait_after_response(uint8_t* buffer)
{
    buffer[0] = 0xff;
    spi_read_write(buffer, 1); // after R1 response wait for 8 clock cycles (for safety)
}
