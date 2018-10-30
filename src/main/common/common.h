#ifndef MEXT2_COMMON_H
#define MEXT2_COMMON_H

#define STATIC static

#define __mext2_packed __attribute__((packed))


// Boolean values
#define MEXT2_TRUE 1
#define MEXT2_FALSE 0

typedef enum mext2_return_value
{
    MEXT2_RETURN_FAILURE = 0,
    MEXT2_RETURN_SUCCESS
} mext2_return_value;

#endif
