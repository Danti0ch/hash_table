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

    InitTesting();

    LogClose();

    return 0;
}
