#include "debug.h"
#include "pin.h"
#include "spi.h"
#include "command.h"
#include "common.h"
#include "sd.h"
#include "crc.h"

#define SDV1X_BLOCK_ADDRESS_SHIFT 9

#define START_BLOCK_TOKEN 0xfe
#define START_BLOCK_TOKEN_MULTIPLE_WRITE 0xfc
#define STOP_BLOCK_TOKEN_MULTIPLE_WRITE 0xfd

STATIC uint8_t single_block_read(uint32_t index, block512_t* block)
{
    //prepare block for read
    memset(block->data, 0xff, 512);

    wait_8_clock_cycles();

    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    mext2_response response = mext2_send_command(COMMAND_READ_SINGLE_BLOCK, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't read block. R1 response: %#hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    if(!wait_for_response((uint8_t*)&response))
        return MEXT2_RETURN_FAILURE;

    if(response.r1 != START_BLOCK_TOKEN)
    {
        mext2_error("Received wrong data token. Token value: %#hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    mext2_spi_read_write(block->data, sizeof(block512_t));

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
        mext2_error("Can't read block. R1 response: %#hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    for(uint8_t i = 0; i < blocks_count; i++)
    {
        if(wait_for_response((uint8_t*)&response) == MEXT2_RETURN_FAILURE)
            return MEXT2_RETURN_FAILURE;

        if(response.r1 != START_BLOCK_TOKEN)
        {
            mext2_error("Received wrong data token. Token value: %#hhx", response.r1);
            return MEXT2_RETURN_FAILURE;
        }

        mext2_spi_read_write(blocks[i].data, sizeof(block512_t));

        wait_8_clock_cycles();
        wait_8_clock_cycles();
    }

    if(wait_for_response((uint8_t*)&response) == MEXT2_RETURN_FAILURE)
        return MEXT2_RETURN_FAILURE;

    memset(command_argument, 0x00, COMMAND_ARGUMENT_SIZE);

    response = mext2_send_command(COMMAND_STOP_READ_DATA, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't read block. R1 response: %#hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
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

    if(sd->sd_version == SD_V1X || sd->sd_version == SD_V2X)
        index <<= SDV1X_BLOCK_ADDRESS_SHIFT;

    mext2_return_value return_value;

    if(blocks_count == 1)
        return_value = single_block_read(index, blocks);
    else
        return_value = multiple_block_read(index, blocks, blocks_count);

    return return_value;
}

//////////////////////////////////////////////////////
#define DATA_RESPONSE_TOKEN_STATUS_MASK 0xe
#define DATA_ACCEPTED 0x4 // 010
#define CRC_ERROR 0xa     // 101
#define WRITE_ERROR 0xc   // 110

STATIC uint8_t single_block_wite(uint32_t index, block512_t* block)
{
    wait_8_clock_cycles();

    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    mext2_response response = mext2_send_command(COMMAND_WRITE_SINGLE_BLOCK, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't write block. R1 response: %#hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    uint8_t token = START_BLOCK_TOKEN;
    uint16_t crc = (uint16_t)crc16(&block->data[0], sizeof(block512_t));
    if(!mext2_is_big_endian())
        crc = mext2_flip_endianess16(crc);
    mext2_spi_read_write(&token, sizeof(token));
    mext2_spi_write(&block->data[0], sizeof(block512_t));
    mext2_spi_read_write((uint8_t*)&crc, sizeof(crc));

    wait_for_response(&token);
    uint8_t status = token & DATA_RESPONSE_TOKEN_STATUS_MASK;

    do
    {
        token = 0xff;
        mext2_spi_read_write(&token, sizeof(token));
    } while(token == 0);

    if(status != DATA_ACCEPTED)
    {
        if(status == CRC_ERROR)
            mext2_error("CRC check error occured during single block write");
        else if(status == WRITE_ERROR)
            mext2_error("Write error occured during single block write");
        else
            mext2_error("Invalid data response token");

        memset(command_argument, 0x00, COMMAND_ARGUMENT_SIZE);
        response = mext2_send_command(COMMAND_SEND_STATUS, command_argument);
        mext2_debug("R2 response %#hhx %#hhx", response.r1, response.extended_response[0]);

        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}

STATIC uint8_t multiple_block_write(uint32_t index, block512_t* blocks, uint8_t blocks_number)
{
    wait_8_clock_cycles();

    uint8_t command_argument[] = {(uint8_t)(index >> 24), (uint8_t)(index >> 16), (uint8_t)(index >> 8), (uint8_t)index};
    mext2_response response = mext2_send_command(COMMAND_WRITE_MULTIPLE_BLOCK, command_argument);
    if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
    {
        mext2_error("Can't write block. R1 response: %#hhx", response.r1);
        return MEXT2_RETURN_FAILURE;
    }

    uint8_t token;
    uint8_t status;
    for(uint8_t i = 0; i < blocks_number; i++)
    {
        wait_8_clock_cycles();
        token = START_BLOCK_TOKEN_MULTIPLE_WRITE;
        uint16_t crc = (uint16_t)crc16(&blocks[i].data[0], sizeof(block512_t));
        if(!mext2_is_big_endian())
            crc = mext2_flip_endianess16(crc);
        mext2_spi_read_write(&token, sizeof(token));
        mext2_spi_write(&blocks[i].data[0], sizeof(block512_t));
        mext2_spi_read_write((uint8_t*)&crc, sizeof(crc));

        if(!wait_for_response(&token))
            return MEXT2_RETURN_FAILURE;

        do
        {
            response.r1 = 0xff;
            mext2_spi_read_write((uint8_t*) &response, 1);
        } while(response.r1 == 0);

        status = token & DATA_RESPONSE_TOKEN_STATUS_MASK;
        if(status != DATA_ACCEPTED)
        {
            if(status == CRC_ERROR)
                mext2_error("CRC check error occured during multiple block write");
            else if(status == WRITE_ERROR)
                mext2_error("Write error occured during multiple block write");
            else
                mext2_error("Invalid data response token");

            break;
        }
    }

    wait_8_clock_cycles();

    if(status != DATA_ACCEPTED)
    {
        memset(command_argument, 0x00, COMMAND_ARGUMENT_SIZE);
        response = mext2_send_command(COMMAND_STOP_READ_DATA, command_argument);
        if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
        {
            mext2_error("Invalid response to STOP_READ_DATA command. R1 response: %#hhx", response.r1);
            return MEXT2_RETURN_FAILURE;
        }

        memset(command_argument, 0x00, COMMAND_ARGUMENT_SIZE);
        response = mext2_send_command(COMMAND_SEND_STATUS, command_argument);
        if((response.r1 & R1_INVALID_RESPONSE) || response.r1 != 0)
        {
            mext2_error("Invalid response to SEND_STATUS command. R1 response: %#hhx", response.r1);
            return MEXT2_RETURN_FAILURE;
        }
        mext2_debug("R2 response %#hhx %#hhx", response.r1, response.extended_response[0]);
    }
    else
    {
        token = STOP_BLOCK_TOKEN_MULTIPLE_WRITE;
        mext2_spi_read_write(&token, sizeof(token));

        wait_8_clock_cycles();
        do
        {
            response.r1 = 0xff;
            mext2_spi_read_write((uint8_t*) &response, 1);
        } while(response.r1 == 0);
    }

    wait_8_clock_cycles();
    if(status != DATA_ACCEPTED)
    {
        return MEXT2_RETURN_FAILURE;
    }
    else
        return MEXT2_RETURN_SUCCESS;
}


uint8_t mext2_write_blocks(mext2_sd* sd, uint32_t index, block512_t* blocks, uint8_t blocks_number)
{
    if(blocks_number < 1)
    {
        mext2_error("Can't write 0 blocks.");
        return MEXT2_RETURN_FAILURE;
    }

    if(sd->sd_version == SD_V1X || sd->sd_version == SD_V2X)
        index <<= SDV1X_BLOCK_ADDRESS_SHIFT;

    uint8_t return_value;

    if(blocks_number == 1)
        return_value = single_block_wite(index, blocks);
    else
        return_value = multiple_block_write(index, blocks, blocks_number);

    return return_value;
}
