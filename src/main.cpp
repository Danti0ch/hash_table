#include <stdio.h>
#include <stdlib.h>
#include "testing.h"
#include "text_storage.h"
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


    text_storage* text = GetStorage(argv[1]);
    if(text == NULL){
        //  TODO: нужна обработка других ошибок 
        printf("unable to open %s\n", argv[1]);
        return 0;
    }

    char** p_words = (char**)calloc(text->n_words, sizeof(char**));

    word* uniq_words = GetUnicalWords(text);

    for(int i = 0; i < text->n_words; i++){

        p_words[i] = uniq_words[i].pt;
    }

    ReduceWords(uniq_words);
    GetSpectralAnalysis(p_words, text->n_words, hash_size, "lol");

    return 0;
}
