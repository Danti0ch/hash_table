#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "structs/text_storage.h"
#include "testing.h"
#include <limits.h>
// __volatile__ ?

int main(const int argc, const char* argv[]){

    LogInit("../logs/");

    uint start_time = clock();    
    InitTesting();
    uint delt = (uint)clock() - start_time;
    printf("total_time: %g secs\n", ((double)delt)/(128 * (double)CLOCKS_PER_SEC));

    LogClose();
    return 0;
}
