#ifndef MEXT2_COMMON_H
#define MEXT2_COMMON_H

#define STATIC static

#define __mext2_packed __attribute__((packed))

typedef enum mext2_return_value
{
    MRXT2_RETURN_SUCCESS,
    MRXT2_RETURN_FAILURE
} mext2_return_value;

#endif