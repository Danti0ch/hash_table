
#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

void open_log_file(const char* name);

void close_log_file();

void to_log(const char* str, ...);

#endif
