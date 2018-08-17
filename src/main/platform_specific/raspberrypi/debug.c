#include <stdio.h>
#include "debug.h"

#define TRUE 1
#define FALSE 0

uint8_t mext2_log_level = WARNING;
uint8_t mext2_is_log_file_initialized = FALSE;
FILE* mext2_log_file = NULL;

char* mext2_level2string[] =
{
    "INFO",
    "DEBUG",
    "WARNING",
    "ERROR"
};

void mext2_initialize_log_file()
{
    mext2_log_file = stderr;
}

void mext2_set_log_file(FILE* file)
{
    mext2_log_file = file;
    if(!mext2_is_log_file_initialized)
        mext2_is_log_file_initialized = TRUE;
}

FILE* mext2_get_log_file()
{
    return mext2_log_file;
}

void mext2_msg(uint8_t level, char* str)
{
    if(!mext2_is_log_file_initialized)
        mext2_initialize_log_file();
    if(level >= mext2_log_level)
        fprintf(mext2_log_file, "%s: %s\n", mext2_level2string[level], str);
}