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

void mext2_initialize_log_file();
void mext2_set_log_file(FILE* file);
void mext2_set_log_level(uint8_t level);
FILE* mext2_get_log_file();

void mext2_msg(uint8_t level, char* str);
#else
#define NOOP (void)0

#define mext2_msg(level, str) NOOP

#define mext2_initialize_log_file() NOOP
#define mext2_set_log_file(file) NOOP
#define mext2_set_log_level(level) NOOP

#endif

#define mext2_log(str) mext2_msg(INFO, str)
#define mext2_debug(str) mext2_msg(DEBUG, str)
#define mext2_warning(str) mext2_msg(WARNING, str)
#define mext2_error(str) mext2_msg(ERROR, str)

#endif
