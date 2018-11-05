#ifndef MEXT2_FILE_H
#define MEXT2_FILE_H
#include "ext2/file.h"

struct mext2_file;
struct mext2_sd;

/* Function types */
typedef uint8_t (*mext2_open_strategy)(struct mext2_file*, struct mext2_sd*, char*, uint16_t);
typedef uint8_t (*mext2_close_strategy)(struct mext2_file*);
typedef uint8_t (*mext2_read_strategy)(struct mext2_file*, void*, size_t);
typedef uint8_t (*mext2_write_strategy)(struct mext2_file*, void*, size_t);
typedef uint8_t (*mext2_seek_strategy)(struct mext2_file*, int);
typedef uint8_t (*mext2_eof_strategy)(struct mext2_file*);

enum mext2_file_mode
{
    MEXT2_READ = 0x1,
    MEXT2_WRITE = 0x2,
    MEXT2_RW = 0x3,
    MEXT2_APPEND = 0x4
};

typedef struct mext2_file
{
    struct mext2_sd* sd;
    uint8_t mode;
    union
    {
        struct mext2_ext2_file ext2;
    } fs_specific;
} mext2_file;

mext2_file* mext2_open(struct mext2_sd* sd, char* path, uint16_t mode);
uint8_t mext2_close(mext2_file* fd, int count);
uint8_t mext2_write(mext2_file* fd, void* buffer, size_t count);
uint8_t mext2_read(mext2_file* fd, void* buffer, size_t count);
uint8_t mext2_seek(mext2_file* fd, int count);
uint8_t mext2_eof(mext2_file* fd);

#endif
