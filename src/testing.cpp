#include "testing.h"
#include "hash_funcs.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// TODO: посмотреть про функциональные языки
// TODO: assert -> обработчик ошибок

void GetSpectralAnalysis(const list_T* data, const size_t n_elems, const size_t htable_size, const char* image_name, const BAR_DRAW_MODE draw_mode, const uint is_show){

    assert(data           != NULL);
    assert(image_name     != NULL);
    assert(htable_size    != 0);
    assert(n_elems        != 0);
    
    FILE* temp_file = fopen(TEMP_FILE_NAME, "w");
    assert(temp_file != NULL);

    fprintf(temp_file, "%d %u\n", draw_mode, is_show);
    fprintf(temp_file, "%lu %lu %u %u\n", n_elems, htable_size, N_HASH_FUNCS/2, N_HASH_FUNCS/2);

    for(uint n_func = 0; n_func < N_HASH_FUNCS; n_func++){

        fprintf(temp_file, "%s\n", hash_funcs[n_func].name);
        fprintf(temp_file, "%s\n", hash_funcs[n_func].descr);

        HTable* htable = NULL;
        HTableInitCustomHash(&htable, htable_size, hash_funcs[n_func].p_func);
        assert(htable != NULL);

        for(uint n_elem = 0; n_elem < n_elems; n_elem++){

            HTableInsert(htable, data[n_elem]);
        }

        for(uint n_list = 0; n_list < htable_size; n_list++){
            if(IsListEmpty(htable, n_list)) continue;
            for(uint n_elem = 0; n_elem < htable->data[n_list]->size; n_elem++){
                fprintf(temp_file, "%u ", n_list);
            }
        }
        fprintf(temp_file, "\n");

        size_t cur_len = 0;
        for(uint n_list = 0; n_list < htable_size; n_list++){

            if(IsListEmpty(htable, n_list)){
                cur_len = 0;
            }
            else{
                cur_len = htable->data[n_list]->size;
            }

            fprintf(temp_file, "%lu ", cur_len);
        }
        fprintf(temp_file, "\n");

        HTableRemove(htable);
        printf("%s loading completed\n", hash_funcs[n_func].name);
    }

    fclose(temp_file);

    // TODO: refactor

    printf("attemp to execute drawing script\n");
    system("python3 ../src/graphics.py");

    printf("succesful\n");
    return;
}
//----------------------------------------------------------------------------------------//

void UseHtable(const text_storage* text, const text_storage* unique_text, const size_t hash_size){
    
    HTable* htable = NULL;
    HTableInit(&htable, hash_size);
    assert(htable != NULL);

    for(uint n_elem = 0; n_elem < unique_text->n_words; n_elem++){
        HTableInsert(htable, unique_text->p_words[n_elem].pt);
    }
    uint is_in_table = 0;
    for(uint n_word = 0; n_word < text->n_words; n_word++){
        HTableFind(htable, text->p_words[n_word].pt, &is_in_table);
    }

    return;
}
//----------------------------------------------------------------------------------------//
