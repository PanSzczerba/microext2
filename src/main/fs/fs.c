#include "common.h"
#include "sd.h"
#include "debug.h"
#include "fs.h"
#include "ext2/ext2_descriptor.h"
#include "ext2/superblock.h"

#define UNIX_SUPERBLOCK_SIZE 1024
#define UNIX_SUPERBLOCK_PARTITION_BLOCK_OFFSET 2
#define MAGIC_OFFSET 56

/******* MAGICS ********/
#define EXT2_SUPER_MAGIC 0x53ef

uint8_t mext2_fs_probe_magic_chain(mext2_sd* sd);

uint8_t (*mext2_fs_probe_chains[MEXT2_FS_PROBE_CHAINS_SIZE])(struct mext2_sd*) = { mext2_fs_probe_magic_chain };

uint8_t mext2_fs_probe_magic_chain(mext2_sd* sd)
{
    block512_t superblock[UNIX_SUPERBLOCK_SIZE / sizeof(block512_t)];

    if(mext2_read_blocks(sd, sd->partition_block_addr + UNIX_SUPERBLOCK_PARTITION_BLOCK_OFFSET,
            superblock, UNIX_SUPERBLOCK_SIZE / sizeof(block512_t)) == MEXT2_RETURN_FAILURE)
    {
        mext2_error("Couldn't read superblock");
        return MEXT2_RETURN_FAILURE;
    }

    uint16_t magic = mext2_le_to_cpu16(*((uint16_t*)(((uint8_t*)superblock) + MAGIC_OFFSET)));
    mext2_debug("FS magic: 0x%hx", magic);

    uint8_t fs_found = MEXT2_TRUE;

    switch(magic)
    {
    case EXT2_SUPER_MAGIC:
        mext2_log("Found Ext2 magic number");
        mext2_ext2_sd_filler(sd, (struct mext2_ext2_superblock*)superblock);
        break;
    default:
        mext2_debug("Couldn't find any known magic number");
        fs_found = MEXT2_FALSE;
    }

    if(fs_found)
        return MEXT2_RETURN_SUCCESS;
    else
        return MEXT2_RETURN_FAILURE;
}
