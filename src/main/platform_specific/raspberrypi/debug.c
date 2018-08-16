#include <stdio.h>
#include "debug.h"

#define TRUE 1
#define FALSE 0

uint8_t _mext2_log_level = WARNING;
uint8_t _mext2_is_log_file_initialized = FALSE;
FILE* _mext2_log_file = NULL;

char* mext2_level2string[] =
{
    "INFO",
    "DEBUG",
    "WARNING",
    "ERROR"
};

void _mext2_initialize_log_file()
{
    _mext2_log_file = stderr;
}

void mext2_set_log_file(FILE* file)
{
    _mext2_log_file = file;
    if(!_mext2_is_log_file_initialized)
        _mext2_is_log_file_initialized = TRUE;
}

FILE* mext2_get_log_file()
{
    return _mext2_log_file;
}
