#ifndef MEXT2_SD_H
#define MEXT2_SD_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common.h"
//#include "fs.h"

#define CSD_REG_SIZE 16

enum mext2_return_value;

typedef struct block512_t
{
    uint8_t data[512];
} block512_t;

typedef enum mext2_sd_version
{
    SD_NOT_DETERMINED,
    SD_V1X,
    SD_V2X,
    SD_V2XHCXC
} mext2_sd_version;

typedef struct mext2_sd
{
    mext2_sd_version sd_version;
    uint8_t sd_initialized;
    uint8_t csd[CSD_REG_SIZE];
    uint64_t fs_block_addr;
    uint8_t fs_type;
    union
    {
//  	Ext2Descriptor ext2;
//  	FATDescriptor fat;

    } fs_specific;
} mext2_sd;

uint8_t mext2_sd_init(mext2_sd* sd);
uint8_t mext2_read_blocks(mext2_sd* sd, uint32_t index, block512_t* block, uint8_t blocks_number);
uint8_t mext2_write_blocks(mext2_sd* sd, uint32_t index, block512_t* block, uint8_t blocks_number);

#endif
