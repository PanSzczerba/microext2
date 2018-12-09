#ifndef MEXT2_FS_EXT2_SUPERBLOCK_H
#define MEXT2_FS_EXT2_SUPERBLOCK_H

#include <stdint.h>
#include "common.h"

#define EXT2_SUPERBLOCK_PARTITION_BLOCK_OFFSET 2 // this is physical block not partition one
#define EXT2_SUPERBLOCK_PHYSICAL_BLOCK_SIZE 2

/**** possible s_state values ****/
#define EXT2_VALID_FS   (uint16_t)1   // Unmounted cleanly
#define EXT2_ERROR_FS   (uint16_t)2   // Errors detected

/**** REVISION NUMBERS ****/
#define EXT2_GOOD_OLD_REV 0
#define EXT2_DYNAMIC_REV 1

/**** EXT2_GOOD_OLD_REV SPECIFIC ****/
#define EXT2_GOOD_OLD_INODE_SIZE 128
#define EXT2_GOOD_OLD_FIRST_INO 11

#define EXT2_NO_FEATURES 0
/**** EXT2 COMPATIBLE FEATURES  ****/
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC    0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES   0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL     0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR        0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INO      0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX       0x0020

#define MEXT2_FEATURES_COMPAT (EXT2_NO_FEATURES)

/**** EXT2 INCOMPATIBLE FEATURES  ****/
#define EXT2_FEATURE_INCOMPAT_COMPRESSION   0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE      0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER       0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV   0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG       0x0010

#define MEXT2_FEATURES_INCOMPAT (EXT2_FEATURE_INCOMPAT_FILETYPE)

/**** EXT2 COMPATIBLE READ-ONLY FEATURES ****/
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE   0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR    0x0004

#define MEXT2_FEATURES_RO_COMPAT (EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER | EXT2_FEATURE_RO_COMPAT_LARGE_FILE)

struct mext2_ext2_superblock
{
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
/*** EXT2_DYNAMIC_REV Specific ***/
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
    char     s_last_mounted[64];
    uint32_t s_algo_bitmap;
/*** Performance Hints ***/
    uint8_t  s_prealloc_blocks;
    uint8_t  s_prealloc_dir_blocks;
    uint16_t _alignment;
/*** Journaling Support ***/
    uint8_t  s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
/*** Directory Indexing Support ***/
    uint32_t s_hash_seed[4];
    uint8_t  s_def_hash_version;
    uint8_t  _padding[3]; /* reserved for future expansion */
/*** Other options ***/
    uint32_t s_default_mount_options;
    uint32_t s_first_meta_bg;
} __mext2_packed;

#endif
