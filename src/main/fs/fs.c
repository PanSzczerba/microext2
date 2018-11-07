#include "common.h"
#include "sd.h"
#include "debug.h"
#include "fs.h"
#include "ext2/ext2.h"
#include "ext2/superblock.h"

#define UNIX_SUPERBLOCK_SIZE 1024
#define UNIX_SUPERBLOCK_PARTITION_BLOCK_OFFSET 2
#define MAGIC_OFFSET 56

/******* MAGICS ********/
#define EXT2_SUPER_MAGIC 0x53ef

uint8_t mext2_fs_probe_magic_chain(mext2_sd* sd);

mext2_probe_fs_strategy mext2_fs_probe_chains[MEXT2_FS_PROBE_CHAINS_SIZE] = { mext2_fs_probe_magic_chain };

uint8_t mext2_fs_probe_magic_chain(mext2_sd* sd)
{
    block512_t* superblock = &mext2_usefull_blocks[0]; // usefull blocks should be at least 1024 bytes in size

    if(mext2_read_blocks(sd, sd->partition_block_addr + UNIX_SUPERBLOCK_PARTITION_BLOCK_OFFSET,
            superblock, UNIX_SUPERBLOCK_SIZE / sizeof(block512_t)) == MEXT2_RETURN_FAILURE)
    {
        mext2_error("Couldn't read superblock");
        return MEXT2_RETURN_FAILURE;
    }

    uint16_t magic = mext2_le_to_cpu16(*((uint16_t*)(((uint8_t*)superblock) + MAGIC_OFFSET)));
    mext2_debug("FS magic: 0x%hx", magic);


    switch(magic)
    {
    case EXT2_SUPER_MAGIC:
        mext2_log("Found Ext2 magic number");
        if(mext2_ext2_sd_parser(sd, (struct mext2_ext2_superblock*)superblock) != MEXT2_RETURN_SUCCESS)
        {
            mext2_error("Couldn't parse ext2 superblock");
            return MEXT2_RETURN_FAILURE;
        }
        break;
    default:
        mext2_error("Couldn't find any known magic number");
        return MEXT2_RETURN_FAILURE;
    }

    return MEXT2_RETURN_SUCCESS;
}
