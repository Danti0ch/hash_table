#include "logger.h"
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static FILE* log_file       = NULL;

static log_location CUR_LOC = {
        "", "", 0
};

//========================================================================================//

//                          LOCAL_FUNCTIONS_DECLARATION

//========================================================================================//

static void get_log_name(char* str, const char* path_to_logs);
static void make_log(const char* string, ...);

//========================================================================================//

//                          PUBLIC_FUNCTIONS_DEFINITION

//========================================================================================//

int LogInit(const char* path_to_logs){

    char name[MAX_LOG_NAME_LEN] = "";
    get_log_name(name, path_to_logs);
    log_file = fopen(name, "w");

    if(log_file == NULL){
        printf("Unable to create log file on way: %s%s\n", path_to_logs, name);
        return 0;
    }

    LOG("Logging initiated");

	return 1;
}
//----------------------------------------------------------------------------------------//

void LogClose(){

    LOG("Logging closed");
    fclose(log_file);

    return;
}
//----------------------------------------------------------------------------------------//

log_func_type log_wrapper(const char* file_name, const char* func_name, const uint n_line){

    CUR_LOC.n_line      = n_line;
    CUR_LOC.file_name   = file_name;
    CUR_LOC.func_name   = func_name;
    
    return &make_log;
}

//========================================================================================//
//
//                          LOCAL_FUNCTIONS_DEFINITION
//
//========================================================================================//

static void get_log_name(char* buf, const char* path_to_logs){

    assert(buf          != NULL);
    assert(path_to_logs != NULL);

	strcpy(buf, path_to_logs);

    time_t rawtime = time(NULL);
    struct tm *ptm = localtime(&rawtime);

    strftime(buf + strlen(buf), MAX_LOG_NAME_LEN, "%H-%M-%S(%d.%m.%Y)", ptm);

    return;
}
//----------------------------------------------------------------------------------------//

void make_log(const char* string, ...){

    assert(string != NULL);
    
    time_t rawtime = time(NULL);
    struct tm *ptm = localtime(&rawtime);

    char time_str[20] = "";
    strftime(time_str, 16, "%H:%M:%S", ptm);

	va_list args;
	va_start(args, string);

    fprintf(log_file, "\n\n%s--%s--%s<%u>:: \n",
        time_str, CUR_LOC.file_name, CUR_LOC.func_name, CUR_LOC.n_line);

    vfprintf(log_file, string, args);
	fprintf(log_file, "\n");

	va_end(args);

    return;
}
//----------------------------------------------------------------------------------------//
