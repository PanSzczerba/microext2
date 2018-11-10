#ifndef MEXT2_COMMON_H
#define MEXT2_COMMON_H

#include <stdint.h>
#include "limit.h"

#define STATIC static

#define __mext2_packed __attribute__((packed))

// Boolean values
#define MEXT2_TRUE 1
#define MEXT2_FALSE 0

typedef uint8_t mext2_bool;

typedef enum mext2_return_value
{
    MEXT2_RETURN_FAILURE = 0,
    MEXT2_RETURN_SUCCESS
} mext2_return_value;


typedef struct block512_t
{
    uint8_t data[512];
} block512_t;

uint8_t mext2_is_big_endian(void);

uint16_t mext2_flip_endianess16(uint16_t num);
uint32_t mext2_flip_endianess32(uint32_t num);
uint64_t mext2_flip_endianess64(uint64_t num);

#define mext2_le_to_cpu16(num) (mext2_is_big_endian() ? mext2_flip_endianess16(num) : (num))
#define mext2_le_to_cpu32(num) (mext2_is_big_endian() ? mext2_flip_endianess32(num) : (num))
#define mext2_le_to_cpu64(num) (mext2_is_big_endian() ? mext2_flip_endianess64(num) : (num))

#define mext2_cpu_to_le16(num) (mext2_is_big_endian() ? mext2_flip_endianess16(num) : (num))
#define mext2_cpu_to_le32(num) (mext2_is_big_endian() ? mext2_flip_endianess32(num) : (num))
#define mext2_cpu_to_le64(num) (mext2_is_big_endian() ? mext2_flip_endianess64(num) : (num))

extern block512_t mext2_usefull_blocks[MEXT2_USEFULL_BLOCKS_SIZE];
extern uint32_t mext2_errno;

/**** MEXT2 ERROR CODES ****/
enum ext2_error_codes
{
    MEXT2_NO_ERRORS = 0,
    /* FILE OPERATIONS ERRORS */
    MEXT2_INVALID_PATH,
    MEXT2_EOF,
    MEXT2_READ_ERROR,
    MEXT2_WRITE_ERROR,
    MEXT2_RO_WRITE_TRY,
    MEXT2_NOT_ENOUGH_FD,
    /* FS ERRORS */
    MEXT2_TOO_LARGE_BLOCK,
    MEXT2_ERRONEOUS_FS,
    MEXT2_READ_ONLY_FS,
    /* this should be last */
    MEXT2_UNKNOWN_ERROR
};

#endif
