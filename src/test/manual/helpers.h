#ifndef MEXT2_TEST_MANUAL_HELPERS_H
#define MEXT2_TEST_MANUAL_HELPERS_H

#include <stdio.h>
#include "debug.h"
#include "sd.h"
#include "command.h"
#include "ext2/ext2.h"
#include "file.h"

#define MAX_BLOCKS_TO_READ 128
#define BLOCK_SIZE 512

void display_buffer(size_t block_address, block512_t* buff)
{
    for(size_t i = 0; i < 32; i++)
    {
        mext2_print("%03zx: ", block_address + i*16);
        for(size_t j = 0; j < 16; j++)
            mext2_print("%02x ", buff->data[i * 16 + j]);
        mext2_print("\n");
    }
}

void display_blocks(mext2_sd* sd, size_t block_address, size_t block_no)
{
    block512_t* buff;
    buff = (block512_t*)malloc(sizeof(block512_t)*block_no);
    mext2_read_blocks(sd, block_address, buff, block_no);
    for(size_t k = 0; k < block_no; k++)
    {
        for(size_t i = 0; i < 32; i++)
        {
            mext2_print("%03zx: ", block_address + k*BLOCK_SIZE + i*16);
            for(size_t j = 0; j < 16; j++)
                mext2_print("%02x ", buff[k].data[i * 16 + j]);
            mext2_print("\n");
        }
    }
}

void display_csd(mext2_sd* sd)
{

    uint8_t* csd_ptr = (uint8_t*)&sd->csd;
    mext2_log("CSD register content: ");
    for(int i = 0; i < sizeof(sd->csd); i++)
    {
        mext2_print("0x%hhx ", csd_ptr[i]);
    }
    mext2_print("\n");
}

void display_sd_version(mext2_sd* sd)
{
    static char* sd_version_strings[] = { "SD_V1X", "SD_V2X", "SD_V2XHCXC" };
    mext2_log("SD version: %s", sd_version_strings[sd->sd_version]);
}
#endif
