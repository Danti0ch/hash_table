/**
 * @file logger.h
 * @brief реализация методы для логирования ошибок, варнингов компиляции этой программы и логирования ошибок, варнингов компилируемой пользовательской программы
 * @version 0.1
 * @date 2022-04-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef LOGGER_H
#define LOGGER_H

struct log_location{

    const char*   file_name;
    const char*   func_name;
    unsigned int  n_line;
};

const int  MAX_LOG_NAME_LEN  = 2 << 8;
const char LOG_LOCATION[32]  = "../logs/";  // TODO: make config for customise this option

typedef void (* log_func_type)(const char* string, ...);

// https://habr.com/ru/post/138150/

#define LOG   log_wrapper(__FILE__, __FUNCTION__, __LINE__)

// extern "C"?
log_func_type log_wrapper(const char* file_name, const char* func_name, const unsigned int n_line);

int LogInit(const char* path_to_logs);
void LogClose();

#endif  // LOGGER_H
