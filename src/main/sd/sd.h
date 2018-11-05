#ifndef MEXT2_SD_H
#define MEXT2_SD_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common.h"
#include "fs.h"

enum mext2_return_value;

typedef struct block512_t
{
    uint8_t data[512];
} block512_t;

typedef enum mext2_sd_version
{
    SD_NOT_DETERMINED = -1,
    SD_V1X,
    SD_V2X,
    SD_V2XHCXC
} mext2_sd_version;

struct mext2_csd_v1
{
    uint8_t csd_structure : 2;
    uint8_t taac : 8;
    uint8_t nsac : 8;
    uint8_t tran_speed : 8;
    uint16_t ccc : 12;
    uint8_t read_bl_len : 4;
    uint8_t read_bl_partial : 1;
    uint8_t write_blk_misalign : 1;
    uint8_t read_blk_misalign : 1;
    uint8_t dsr_imp : 1;
    uint16_t c_size : 12;
    uint8_t vdd_r_curr_min : 3;
    uint8_t vdd_r_curr_max : 3;
    uint8_t vdd_w_curr_min : 3;
    uint8_t vdd_w_curr_max : 3;
    uint8_t c_size_mult : 3;
    uint8_t erase_blk_en : 1;
    uint8_t sector_size : 7;
    uint8_t wp_grp_size : 7;
    uint8_t wp_grp_enable : 1;
    uint8_t r2w_factor : 3;
    uint8_t write_bl_len : 4;
    uint8_t write_bl_partial : 1;
    uint8_t file_format_grp : 1;
    uint8_t copy : 1;
    uint8_t perm_write_protect : 1;
    uint8_t tmp_write_protect : 1;
    uint8_t file_format : 2;
    uint8_t csd_crc : 8;
} __mext2_packed;

struct mext2_csd_v2
{
    uint8_t csd_structure : 2;
    uint8_t taac : 8;
    uint8_t nsac : 8;
    uint8_t tran_speed : 8;
    uint16_t ccc : 12;
    uint8_t read_bl_len : 4;
    uint8_t read_bl_partial : 1;
    uint8_t write_blk_misalign : 1;
    uint8_t read_blk_misalign : 1;
    uint8_t dsr_imp : 1;
    uint32_t c_size : 22;
    uint8_t erase_blk_en : 1;
    uint8_t sector_size : 7;
    uint8_t wp_grp_size : 7;
    uint8_t wp_grp_enable : 1;
    uint8_t r2w_factor : 3;
    uint8_t write_bl_len : 4;
    uint8_t write_bl_partial : 1;
    uint8_t file_format_grp : 1;
    uint8_t copy : 1;
    uint8_t perm_write_protect : 1;
    uint8_t tmp_write_protect : 1;
    uint8_t file_format : 2;
    uint8_t csd_crc : 8;
} __mext2_packed;

union mext2_csd
{
    struct mext2_csd_v1 csd_v1;
    struct mext2_csd_v2 csd_v2;
};

typedef struct mext2_sd
{
    mext2_sd_version sd_version;
    uint8_t sd_initialized;
    union mext2_csd csd;
    uint32_t partition_block_addr;
    uint32_t partition_block_size;
    struct mext2_fs_descriptor fs;
} mext2_sd;

uint8_t mext2_sd_init(mext2_sd* sd);
uint8_t mext2_read_blocks(mext2_sd* sd, uint32_t index, block512_t* block, uint8_t blocks_number);
uint8_t mext2_write_blocks(mext2_sd* sd, uint32_t index, block512_t* block, uint8_t blocks_number);

#endif
