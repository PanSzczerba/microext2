#include "sd.h"
#include "debug.h"
#include "pin.h"
#include "spi.h"
#include "commands.h"

mext2_command* command;

uint8_t single_block_read(uint32_t index, block512_t* block)
{
    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    set_command(command, COMMAND_READ_SINGLE_BLOCK, command_argument);    //set CMD17

    //prepare block for read
    memset(block->data, 0xff, 512);

    uint8_t buffer = 0xff;
    wait_8_clock_cycles(&buffer);

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL || response -> r1 != 0)
    {
        mext2_error("Can't read block");
        return 1;
    }

    if(wait_for_response((uint8_t*)response) == false)
        return 1;

    if(response -> r1 != 0xfe)
    {
        mext2_error("Received wrong data token");
        return 1;
    }

    spi_read_write(block->data, 512);

    wait_8_clock_cycles(&buffer);
    wait_8_clock_cycles(&buffer);
    wait_8_clock_cycles(&buffer);

    return 0;
}

uint8_t multiple_block_read(uint32_t index, block512_t* block, uint8_t blocks_number)
{
    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    set_command(command, COMMAND_READ_MULTIPLE_BLOCK, command_argument);    //set CMD18

    //prepare block for read
    memset(block->data, 0xff, 512 * blocks_number);

    uint8_t buffer = 0xff;
    wait_8_clock_cycles(&buffer);

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL || response -> r1 != 0)
    {
        mext2_error("Can't read block");
        return 1;
    }

    for(uint8_t i = 0; i < blocks_number; i++)
    {
        if(wait_for_response((uint8_t*)response) == false)
            return 1;

        if(response -> r1 != 0xfe)
        {
            mext2_error("Received wrong data token");
            return 1;
        }

        spi_read_write(block[i].data, 512);

        wait_8_clock_cycles(&buffer);
        wait_8_clock_cycles(&buffer);
    }

    if(wait_for_response((uint8_t*)response) == false)
        return 1;

    memset(command_argument, 0x00, COMMAND_ARGUMENT_SIZE);
    set_command(command, COMMAND_STOP_READ_DATA, command_argument);    //set CMD12

    response = send_command(command, MEXT2_R1b);
    if(response == NULL || response -> r1 != 0)
    {
        mext2_error("Can't read block");
        return 1;
    }

    wait_8_clock_cycles(&buffer);

    buffer = 0;
    for(uint8_t i = 0; buffer == 0; i++)
    {
        wait_8_clock_cycles(&buffer);
    }

    wait_8_clock_cycles(&buffer);
    return 0;
}



uint8_t read_blocks(uint8_t blocks_number, uint32_t index, block512_t* block)
{
    if(blocks_number < 1)
    {
        mext2_error("Can't read 0 blocks.");
        return 1;
    }

    command = (mext2_command*)malloc(sizeof(mext2_command));
    uint8_t return_value;

    if(blocks_number == 1)
    {
        return_value = single_block_read(index, block);
    }
    else
    {
        return_value = multiple_block_read(index,block,blocks_number);
    }

    free(command);
    return return_value;
}



//////////////////////////////////////////////////////

uint8_t single_block_wite(uint32_t index, block512_t* block)
{
    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    set_command(command, COMMAND_WRITE_SINGLE_BLOCK, command_argument);    //set CMD24

    uint8_t buffer = 0xff;
    wait_8_clock_cycles(&buffer);

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL || response -> r1 != 0)
    {
        mext2_error("Can't write block.");
        return 1;
    }

    wait_8_clock_cycles(&buffer);
    wait_8_clock_cycles(&buffer);

    spi_read_write(block -> data, 512);

    wait_8_clock_cycles(&buffer);
    if(wait_for_response((uint8_t*) response) == false)
        return 1;

    response -> r1 = 0;
    while(response -> r1 != 0xff)
    {
        response -> r1 = 0xff;
        spi_read_write((uint8_t*) response, 1);
    }
    return 0;
}

uint8_t multiple_block_write(uint32_t index, block512_t* block, uint8_t blocks_number)
{
     uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    set_command(command, COMMAND_WRITE_MULTIPLE_BLOCK, command_argument);    //set CMD25

    uint8_t buffer = 0xff;
    wait_8_clock_cycles(&buffer);

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL || response -> r1 != 0)
    {
        mext2_error("Can't write block.");
        return 1;
    }

    for(uint8_t i = 0; i < blocks_number; i++)
    {
        wait_8_clock_cycles(&buffer);
        wait_8_clock_cycles(&buffer);
        spi_read_write(block[i].data, 512);

        if(wait_for_response((uint8_t*) response) == false)
            return 1;

        response -> r1 = 0;
        while(response -> r1 == 0)
        {
            response -> r1 = 0xff;
            spi_read_write((uint8_t*) response, 1);
        }
    }

    wait_8_clock_cycles(&buffer);

    memset(command_argument, 0x00, COMMAND_ARGUMENT_SIZE);
    set_command(command, COMMAND_STOP_READ_DATA, command_argument);    //set CMD12

    response = send_command(command, MEXT2_R1b);
    if(response == NULL || response -> r1 != 0)
    {
        mext2_error("Can't read block.");
        return 1;
    }

    response -> r1 = 0;
    while(response -> r1 == 0)
    {
        response -> r1 = 0xff;
        spi_read_write((uint8_t*) response, 1);
    }

    wait_8_clock_cycles(&buffer);
    return 0;
}


uint8_t write_blocks(uint8_t blocks_number, uint32_t index, block512_t* block)
{
    if(blocks_number < 1)
    {
        mext2_error("Can't write 0 blocks.");
        return 1;
    }

    command = (mext2_command*)malloc(sizeof(mext2_command));
    block512_t* block_to_write = malloc(sizeof(block512_t) * blocks_number);
    memcpy(block_to_write, block, sizeof(block512_t) * blocks_number);

    uint8_t return_value;

    if(blocks_number == 1)
    {
        return_value = single_block_wite(index, block_to_write);
    }
    else
    {
        return_value = multiple_block_write(index,block,blocks_number);
    }

    free(command);
    free(block_to_write);
    return return_value;
}

