#ifndef MEXT2_SD_H
#define MEXT2_SD_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common.h"

enum mext2_return_value;

#define OK  (uint8_t)0x01
#define NOK (uint8_t)0x00

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
} mext2_sd;

enum mext2_return_value mext2_sd_init(mext2_sd* sd);
enum mext2_return_value mext2_read_blocks(mext2_sd* sd, uint32_t index, block512_t* block, uint8_t blocks_number);
enum mext2_return_value mext2_write_blocks(mext2_sd* sd, uint32_t index, block512_t* block, uint8_t blocks_number);
#endif
