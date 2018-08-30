#include "sd.h"
#include "debug.h"
#include "pin.h"
#include "spi.h"

#define COMMAND_READ_SINGLE_BLOCK       (uint8_t)0x11   //CMD17
#define COMMAND_READ_MULTIPLE_BLOCK     (uint8_t)0x12   //CMD18
#define COMMAND_WRITE_SINGLE_BLOCK      (uint8_t)0x18   //CMD24
#define COMMAND_WRITE_MULTIPLE_BLOCK    (uint8_t)0x19   //CMD25
#define COMMAND_STOP_READ_DATA          (uint8_t)0x0c   //CMD12

typedef struct block512_t
{
    uint8_t data[512];
} block512_t;

mext2_command* command;


void single_block_read(uint32_t index, block512_t* block)
{
    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    set_command(command, COMMAND_READ_SINGLE_BLOCK, command_argument);    //set CMD17

    //prepare block for read
    memset(block->data, 0xff, 512);

    uint8_t buffer = 0xff;
    wait_after_response(&buffer);

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL || response -> r1 != 0)
    {
        debug("Error: can't read block");
        return;
    }

    if(wait_for_response((uint8_t*)response) == false)
        return;

    if(response -> r1 != 0xfe)
    {
        debug("Error: received wrong data token");
        return;
    }

    spi_read_write(block->data, 512);

    wait_after_response(&buffer);
    wait_after_response(&buffer);
    wait_after_response(&buffer);

    free(command);
}

void multiple_block_read(uint32_t index, block512_t* block, uint8_t blocks_number)
{
    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    set_command(command, COMMAND_READ_MULTIPLE_BLOCK, command_argument);    //set CMD18

    //prepare block for read
    memset(block->data, 0xff, 512 * blocks_number);

    uint8_t buffer = 0xff;
    wait_after_response(&buffer);

    mext2_response* response = send_command(command, MEXT2_R1);
    if(response == NULL || response -> r1 != 0)
    {
        debug("Error: can't read block");
        return;
    }

    for(uint8_t i = 0; i < blocks_number; i++)
    {
        if(wait_for_response((uint8_t*)response) == false)
            return;

        if(response -> r1 != 0xfe)
        {
            debug("Error: received wrong data token");
            return;
        }

        spi_read_write(block[i].data, 512);

        wait_after_response(&buffer);
        wait_after_response(&buffer);
    }

    if(wait_for_response((uint8_t*)response) == false)
            return;

    memset(command_argument, 0x00, COMMAND_ARGUMENT_SIZE);
    set_command(command, COMMAND_STOP_READ_DATA, command_argument);    //set CMD12

    response = send_command(command, MEXT2_R1);
    if(response == NULL || response -> r1 != 0)
    {
        debug("Error: can't read block");
        return;
    }

    wait_after_response(&buffer);

    buffer = 0;
    for(uint8_t i = 0; buffer == 0; i++)
    {
        wait_after_response(&buffer);
    }

    wait_after_response(&buffer);
}



uint8_t read_blocks(uint8_t blocks_number, uint32_t index, block512_t* block)
{
    if(blocks_number < 1)
    {
        debug("Error: can't read 0 blocks\n");
        return 1;
    }

    command = (mext2_command*)malloc(sizeof(mext2_command));

    if(blocks_number == 1)
    {
        single_block_read(index, block);
    }
    else
    {
        multiple_block_read(index,block,blocks_number);
    }

    free(command);
    return 0;
}