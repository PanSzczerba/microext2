#include "debug.h"
#include "common.h"

#define MAX_MESSAGE_LEN 128

STATIC uint8_t mext2_log_level = WARNING;
STATIC uint8_t mext2_is_log_file_initialized = MEXT2_FALSE;
STATIC FILE* mext2_log_file = NULL;

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
    mext2_is_log_file_initialized = MEXT2_TRUE;
}

void mext2_set_log_file(FILE* file)
{
    mext2_log_file = file;
    mext2_is_log_file_initialized = MEXT2_TRUE;
}

void mext2_set_log_level(uint8_t level)
{
    mext2_log_level = level;
}

FILE* mext2_get_log_file()
{
    return mext2_log_file;
}

void mext2_msg(uint8_t level, char* str,...)
{
    char buffer[MAX_MESSAGE_LEN];
    va_list args;
    va_start(args, str);
    vsprintf(buffer, str, args);
    va_end(args);
    if(!mext2_is_log_file_initialized)
        mext2_initialize_log_file();
    if(level >= mext2_log_level)
        fprintf(mext2_log_file, "%s: %s\n", mext2_level2string[level], buffer);
}
