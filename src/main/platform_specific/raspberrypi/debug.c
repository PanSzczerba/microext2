#include <stdio.h>
#include "debug.h"

#define TRUE 1
#define FALSE 0

uint8_t _log_level = WARNING;
uint8_t _is_log_file_initialized = FALSE;
FILE* _log_file = NULL;

char* level2string[] =
{
    "INFO",
    "DEBUG",
    "WARNING",
    "ERROR"
};

void _initialize_log_file()
{
    _log_file = stderr;
}

void set_log_file(FILE* file)
{
    _log_file = file;
    if(!_is_log_file_initialized)
        _is_log_file_initialized = TRUE;
}

FILE* get_log_file()
{
    return _log_file;
}
