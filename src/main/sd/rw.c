#include "debug.h"
#include "pin.h"
#include "spi.h"
#include "command.h"
#include "common.h"
#include "sd.h"

#define SDV1X_BLOCK_ADDRESS_SHIFT 9

STATIC uint8_t single_block_read(uint32_t index, block512_t* block)
{
    //prepare block for read
    memset(block->data, 0xff, 512);

    wait_8_clock_cycles();

    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    mext2_response response = mext2_send_command(COMMAND_READ_SINGLE_BLOCK, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't read block. R1 response: 0x%hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    if(!wait_for_response((uint8_t*)&response))
        return MEXT2_RETURN_FAILURE;

    if(response.r1 != 0xfe)
    {
        mext2_error("Received wrong data token. Token value: 0x%hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    spi_read_write(block->data, 512);

    wait_8_clock_cycles();
    wait_8_clock_cycles();
    wait_8_clock_cycles();

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t multiple_block_read(uint32_t index, block512_t* blocks, uint8_t blocks_count)
{
    //prepare block for read
    memset(blocks->data, 0xff, 512 * blocks_count);

    wait_8_clock_cycles();

    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    mext2_response response = mext2_send_command(COMMAND_READ_MULTIPLE_BLOCK, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't read block. R1 response: 0x%hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    for(uint8_t i = 0; i < blocks_count; i++)
    {
        if(wait_for_response((uint8_t*)&response) == MEXT2_RETURN_FAILURE)
            return MEXT2_RETURN_FAILURE;

        if(response.r1 != 0xfe)
        {
            mext2_error("Received wrong data token. Token value: 0x%hhx", response.r1);
            return MEXT2_RETURN_FAILURE;
        }

        spi_read_write(blocks[i].data, 512);

        wait_8_clock_cycles();
        wait_8_clock_cycles();
    }

    if(wait_for_response((uint8_t*)&response) == MEXT2_RETURN_FAILURE)
        return MEXT2_RETURN_FAILURE;

    memset(command_argument, 0x00, COMMAND_ARGUMENT_SIZE);

    response = mext2_send_command(COMMAND_STOP_READ_DATA, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't read block. R1 response: 0x%hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    wait_8_clock_cycles();

    uint8_t buffer = 0;
    for(uint8_t i = 0; buffer == 0; i++)
    {
        wait_8_clock_cycles_with_buffer(&buffer);
    }

    wait_8_clock_cycles();
    return MEXT2_RETURN_SUCCESS;
}


uint8_t mext2_read_blocks(mext2_sd* sd, uint32_t index, block512_t* blocks, uint8_t blocks_count)
{
    if(blocks_count < 1)
    {
        mext2_error("Can't read 0 blocks.");
        return MEXT2_RETURN_FAILURE;
    }

    if(sd->sd_version == SD_V1X)
        index <<= SDV1X_BLOCK_ADDRESS_SHIFT;

    mext2_return_value return_value;

    if(blocks_count == 1)
        return_value = single_block_read(index, blocks);
    else
        return_value = multiple_block_read(index, blocks, blocks_count);

    return return_value;
}

//////////////////////////////////////////////////////

STATIC uint8_t single_block_wite(uint32_t index, block512_t* block)
{
    wait_8_clock_cycles();

    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    mext2_response response = mext2_send_command(COMMAND_WRITE_SINGLE_BLOCK, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't write block. R1 response: 0x%hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    wait_8_clock_cycles();
    wait_8_clock_cycles();

    spi_read_write(block -> data, 512);

    wait_8_clock_cycles();
    if(!wait_for_response((uint8_t*)&response))
        return MEXT2_RETURN_FAILURE;

    do
    {
        response.r1 = 0xff;
        spi_read_write((uint8_t*) &response, 1);
    } while(response.r1 != 0xff);

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t multiple_block_write(uint32_t index, block512_t* blocks, uint8_t blocks_number)
{
    wait_8_clock_cycles();

    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    mext2_response response = mext2_send_command(COMMAND_WRITE_MULTIPLE_BLOCK, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't write block. R1 response: 0x%hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    for(uint8_t i = 0; i < blocks_number; i++)
    {
        wait_8_clock_cycles();
        wait_8_clock_cycles();
        spi_read_write(blocks[i].data, sizeof(block512_t));

        if(!wait_for_response((uint8_t*) &response))
            return MEXT2_RETURN_FAILURE;

        do
        {
            response.r1 = 0xff;
            spi_read_write((uint8_t*) &response, 1);
        } while(response.r1 == 0);
    }

    wait_8_clock_cycles();

    memset(command_argument, 0x00, COMMAND_ARGUMENT_SIZE);

    response = mext2_send_command(COMMAND_STOP_READ_DATA, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't read block. R1 response: 0x%hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    do
    {
        response.r1 = 0xff;
        spi_read_write((uint8_t*) &response, 1);
    } while(response.r1 == 0);


    wait_8_clock_cycles();
    return MEXT2_RETURN_SUCCESS;
}


uint8_t mext2_write_blocks(mext2_sd* sd, uint32_t index, block512_t* blocks, uint8_t blocks_number)
{
    if(blocks_number < 1)
    {
        mext2_error("Can't write 0 blocks.");
        return MEXT2_RETURN_FAILURE;
    }

    if(sd->sd_version == SD_V1X)
        index <<= SDV1X_BLOCK_ADDRESS_SHIFT;

    uint8_t return_value;

    if(blocks_number == 1)
        return_value = single_block_wite(index, blocks);
    else
        return_value = multiple_block_write(index, blocks, blocks_number);

    return return_value;
}
