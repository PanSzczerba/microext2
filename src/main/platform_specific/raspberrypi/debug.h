#ifndef MICROEXT2_DEBUG_H
#define MICROEXT2_DEBUG_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

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

void mext2_msg(uint8_t level, char* str, ...);
void mext2_print(char* str, ...);
#else
#define NOOP (void)0

#define mext2_msg(level, str, ...) NOOP
#define mext2_print(...) NOOP

#define mext2_initialize_log_file() NOOP
#define mext2_set_log_file(file) NOOP
#define mext2_set_log_level(level) NOOP

#endif

#define mext2_log(...) mext2_msg(INFO, __VA_ARGS__)
#define mext2_debug(...) mext2_msg(DEBUG, __VA_ARGS__)
#define mext2_warning(...) mext2_msg(WARNING, __VA_ARGS__)
#define mext2_error(...) mext2_msg(ERROR, __VA_ARGS__)

#endif
