#ifndef MICROEXT2_DEBUG_H
#define MICROEXT2_DEBUG_H

#include <stdio.h>
#include <stdint.h>

enum mext2_log_level
{
    INFO = 0,
    DEBUG,
    WARNING,
    ERROR,
//...........
    NOLOG
};

#ifdef MEXT2_MSG_DEBUG
extern uint8_t _mext2_log_level;
extern FILE* _mext2_log_file;
extern uint8_t _mext2_is_log_file_initialized;

extern char* mext2_level2string[];
void _mext2_initialize_log_file();
void mext2_set_log_file(FILE* file);
FILE* mext2_get_log_file();

#define mext2_msg(level, str) if(!_mext2_is_log_file_initialized) _mext2_initialize_log_file(); \
    if(level >= _mext2_log_level) fprintf(_mext2_log_file, "%s: %s\n", mext2_level2string[level], str) // void msg(uint8_t level, char* str)
#else
#define NOOP (void)0
#define mext2_msg(level, str) NOOP

#endif

#define mext2_log(str) mext2_msg(INFO, str)
#define mext2_debug(str) mext2_msg(DEBUG, str)
#define mext2_warning(str) mext2_msg(WARNING, str)
#define mext2_error(str) mext2_msg(ERROR, str)

#endif
