#include "testing.h"
#include "hash_funcs.h"
#include <stdlib.h>
#include <stdio.h>

// TODO: посмотреть про функциональные языки
// TODO: assert -> обработчик ошибок

void GetSpectralAnalysis(const list_T* data, const size_t n_elems, const size_t htable_size, const char* image_name){

    assert(data           != NULL);
    assert(image_name     != NULL);
    assert(htable_size    != 0);
    assert(n_elems        != 0);
    
    size_t* list_lengths = (size_t*)calloc(htable_size, sizeof(size_t));
    assert(list_lengths != NULL);

    FILE* temp_file = fopen(TEMP_FILE_NAME, "w");
    assert(temp_file != NULL);

    fprintf(temp_file, "%lu %lu %u\n", n_elems, htable_size, N_HASH_FUNCS);

    for(uint n_func = 0; n_func < N_HASH_FUNCS; n_func++){

        fprintf(temp_file, "%s\n", hash_funcs[n_func].name);
        fprintf(temp_file, "%s\n", hash_funcs[n_func].descr);

        Htabl* htable = HTableInit(htable_size, hash_funcs[n_func].p_func);
        assert(htable != NULL);

        for(uint n_elem = 0; n_elem < n_elems; n_elem++){
            HTableInsert(htable, data[n_elem]);
        }
        
        for(uint n_list = 0; n_list < htable_size; n_list++){

            if(IsListEmpty(htable, n_list)){
                list_lengths[n_list] = 0;
            }
            else{
                list_lengths[n_list] = htable->data[n_list]->size;
            }

            fprintf(temp_file, "%lu ", list_lengths[n_list]);
        }
        fprintf(temp_file, "\n");

        HTableRemove(htable);
    }

    fclose(temp_file);
    free(list_lengths);

    // TODO: refactor
    system("python3 ../src/graphics.py");

    return;
}
//----------------------------------------------------------------------------------------//

void GetData(const char* data_file_name){

    assert(data_file_name != NULL);

    FILE* data_file = fopen(data_file_name, "r");

    storage* = TextStorageInit()
}
//----------------------------------------------------------------------------------------//
