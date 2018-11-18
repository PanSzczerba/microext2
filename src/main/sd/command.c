#include "spi.h"
#include "crc.h"
#include "command.h"

#include "debug.h"
#include "pin.h"

#include <stdlib.h>
#include <string.h>
#include "sd.h"

enum response_type
{
    R1,
    R1b,
    R2,
    R3,
    R7
};


uint8_t wait_for_response(uint8_t* buffer)
{
    uint16_t i;
    *buffer = 0xff;
    for(i = 0; i < N_CYCLES_TIMEOUT && *buffer == 0xff; i++)
    {
        *buffer = 0xff;
        spi_read_write(buffer, 1);
    }

    if(i >= N_CYCLES_TIMEOUT)
    {
        mext2_error("Error: exceeded time limit waiting for response, check your SD card reader device.");
        return MEXT2_RETURN_FAILURE;
    }
    return MEXT2_RETURN_SUCCESS;
}

void wait_8_clock_cycles_with_buffer(uint8_t* buffer)
{
    spi_read_write(buffer, 1);
}

void wait_8_clock_cycles()
{
    uint8_t dummy = 0xff;
    spi_read_write(&dummy, 1);
}

STATIC enum response_type get_response_type(uint8_t command_number)
{
    switch(command_number)
    {
    case COMMAND_SOFTWARE_RESET:
        return R1;
    case COMMAND_CHECK_VOLTAGE:
        return R7;
    case COMMAND_READ_OCR:
        return R3;
    case COMMAND_BEFORE_INIT_PROCESS:
        return R1;
    case COMMAND_INIT_PROCESS:
        return R1;
    case COMMAND_READ_CSD:
        return R1;
    case COMMAND_READ_SINGLE_BLOCK:
        return R1;
    case COMMAND_READ_MULTIPLE_BLOCK:
        return R1;
    case COMMAND_WRITE_SINGLE_BLOCK:
        return R1;
    case COMMAND_WRITE_MULTIPLE_BLOCK:
        return R1;
    case COMMAND_STOP_READ_DATA:
        return R1b;
    case COMMAND_SEND_STATUS:
        return R2;
    default:
        return R1;
    }
}

STATIC uint8_t calc_command_number(uint8_t number)
{
    return (number | 0x40);
}

 
STATIC void set_command(mext2_command* command, uint8_t command_number, uint8_t command_argument[COMMAND_ARGUMENT_SIZE])
{
    command->index = calc_command_number(command_number);
    for(uint8_t i = 0; i < COMMAND_ARGUMENT_SIZE; i++)
        command->argument[i] = command_argument[i];
    command->crc = (((crc7((uint8_t*)command, COMMAND_SIZE - sizeof(uint8_t))) << 1) | 1);
}

mext2_response mext2_send_command(uint8_t command_number, uint8_t command_argument[COMMAND_ARGUMENT_SIZE])
{
    mext2_command command;
    set_command(&command, command_number, command_argument);
    mext2_log("Command number sent: %hhu, command body: %#hhx %#hhx %#hhx %#hhx %#hhx %#hhx", command_number,
            command.index, command.argument[0], command.argument[1], command.argument[2], command.argument[3], command.crc);
    spi_read_write((uint8_t*) &command, COMMAND_SIZE);

    enum response_type response_type = get_response_type(command_number);

    mext2_response response = {0xff, { 0xff, 0xff, 0xff, 0xff } };

    if(response_type == R1b)
    {
        wait_8_clock_cycles();
    }

    if(wait_for_response((uint8_t*) &response))
    {
        mext2_log("Response: %#hhx", response.r1);
        if(response_type == R7 || response_type == R3)
            spi_read_write(&response.extended_response[0], 4);
        else if(response_type == R2)
            spi_read_write(&response.extended_response[0], 1);
        wait_8_clock_cycles();
    }

    if(response_type == R1b)
    {
        uint8_t dummy;
        do
        {
            dummy = 0xff;
            wait_8_clock_cycles_with_buffer(&dummy);
        } while(dummy == 0);
        wait_8_clock_cycles();
    }

    return response;
}
