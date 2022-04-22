#include <stdio.h>
#include <stdlib.h>
#include "testing.h"
#include "text_storage.h"
#include "hash_funcs.h"
#include <string.h>

int main(const int argc, const char* argv[]){
    
    if(argc < 3){
        //TODO:
        printf("format should be ...\n");
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
        //  TODO: нужна обработка других ошибок 
        printf("unable to open %s\n", argv[1]);
        return 0;
    }
    
    text_storage* unique_text = GetStorage("unique_words.txt");
    
    
    char** p_words = (char**)calloc(unique_text->n_words, sizeof(char**));

    for(int i = 0; i < unique_text->n_words; i++){
        p_words[i] = unique_text->p_words[i].pt;
    }

    GetSpectralAnalysis(p_words, unique_text->n_words, hash_size, "lol", one_image, 0);    
    
    //UseHtable(text, unique_text, hash_size);
    
    LogClose();
    return 0;
}
