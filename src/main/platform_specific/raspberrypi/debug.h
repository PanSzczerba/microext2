#ifndef MICROEXT2_DEBUG_H
#define MICROEXT2_DEBUG_H

#include <stdio.h>
#include <stdint.h>

enum log_level
{
    INFO = 0,
    DEBUG,
    WARNING,
    ERROR,
//...........
    NOLOG
};

#ifdef DEBUG
extern uint8_t _log_level;
extern FILE* _log_file;
extern uint8_t _is_log_file_initialized

extern char** level2string;
void _initialize_log_file();
void set_log_file(FILE* file);
FILE* get_log_file();

#define msg(level, str) if(!_is_log_file_initialized) _initialize_log_file(); \
    if(level >= _log_level) fprintf(_log_file, "%s: %s\n", level2string[level], str) // void msg(uint8_t level, char* str)
#else
#define NOOP (void)0
#define msg(level, str) NOOP

#endif
#define log(str) msg(INFO, str)
#define debug(str) msg(DEBUG, str)
#define warning(str) msg(WARNING, str)
#define error(str) msg(ERROR, str)

#endif
