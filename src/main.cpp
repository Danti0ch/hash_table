#include <stdio.h>
#include <stdlib.h>
#include "testing.h"
#include <string.h>

int main(){

    open_log_file("log.htm");

    char** strs = (char**)calloc(1000, sizeof(char**));

    for(int i = 0; i < 999; i++){

        char* str = (char*)calloc(100, sizeof(char));

        snprintf(str, sizeof(str), "%d", i);

        strs[i] = str;
    }

    char* str = (char*)calloc(100, sizeof(char));
    str = "''''''''''";

    strs[999] = str;
    
    GetSpectralAnalysis(strs, 1000, 101, "lol");
    close_log_file();

    return 0;
}
