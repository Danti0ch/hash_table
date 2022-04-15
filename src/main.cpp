#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "hash_table.h"
#include "hash_funcs.h"

int main(){

    open_log_file("log.htm");

    Htabl* table =  HTableInit(101, HashReturn0);
    HTableInsert(table, "1234");
    HTableInsert(table, "55");
    HTableInsert(table, "asdf");
    
    uint q = HTableFind(table, "23fd");

    close_log_file();

    return 0;
}
