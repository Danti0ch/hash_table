#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "text_storage.h"
#include "testing.h"
#include <limits.h>
// __volatile__ ?

int main(const int argc, const char* argv[]){
    
    if(argc < 3){
        printf("format should be: ./main file_name hash_init_size\n");
        return 0;
    }

    uint hash_size = atoi(argv[2]);
    if(hash_size == 0){
        printf("hash_size must be in range [1, %u]\n", MAX_HASH_SIZE);
        return 0;
    }

    if(argc >= 4){
        if(!LogInit(argv[3])){
            printf("unable to create log on %s path", argv[3]);
            return 0;
        }
    }
    else{
        LogInit("../logs/");
    }

    text_storage* text = GetStorage(argv[1]);

    if(text == NULL){
        printf("unable to open %s\n", argv[1]);
        return 0;
    }
    
    text_storage* unique_text = GetStorage("unique_words.txt");

    char** p_words = (char**)calloc(unique_text->n_words, sizeof(char**));

    for(int i = 0; i < unique_text->n_words; i++){
        p_words[i] = unique_text->p_words[i].pt;        
    }

    uint start_time = clock();
    for(uint i = 0; i < 50; i++){
        UseHtable(text, unique_text, hash_size);
    }

    uint delt = (uint)clock() - start_time;
    
    printf("total_time: %u\n", delt);
    LogClose();
    return 0;
}
