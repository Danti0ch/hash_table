#include "testing.h"
#include "hash_funcs.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

void GetSpectralAnalysis(const text_storage* data, const size_t htable_size, const BAR_DRAW_MODE save_mode, const uint is_show){
    
    assert(data           != NULL);
    assert(htable_size    != 0);
    
    FILE* temp_file = fopen(TEMP_FILE_NAME, "w");
    assert(temp_file != NULL);

    fprintf(temp_file, "%d %u\n", draw_mode, is_show);
    fprintf(temp_file, "%lu %lu %u %u\n", data->n_words, htable_size, N_HASH_FUNCS/2, N_HASH_FUNCS/2);

    for(uint n_func = 0; n_func < N_HASH_FUNCS; n_func++){

        printf("Insertion of %s\n", hash_funcs[n_func].name);

        fprintf(temp_file, "%s\n", hash_funcs[n_func].name);
        fprintf(temp_file, "%s\n", hash_funcs[n_func].descr);

        HTable* htable = NULL;
        HTableInitCustomHash(&htable, htable_size, hash_funcs[n_func].p_func);
        
        assert(htable != NULL);

        for(uint n_elem = 0; n_elem < n_elems; n_elem++){
            HTableInsert(htable, GetWord(data, n_elem), 0);
        }

        htable_size = htable->size;
        printf("Writing spectral data 1\n");

        for(uint n_list = 0; n_list < htable_size; n_list++){
            if(IsListEmpty(htable, n_list)) continue;

            size_t cur_list_size = GetListSize(htable, n_list);

            for(uint n_word = 0; n_word < cur_list_size; n_word++){
                fprintf(temp_file, "%u ", n_list);
            }
        }
        fprintf(temp_file, "\n");

        printf("Writing spectral data 2\n");

        for(uint n_list = 0, size_t cur_len = 0; n_list < htable_size; n_list++){

            if(IsListEmpty(htable, n_list)) cur_len = 0;
            else{
                cur_len = GetListSize(htable, n_list);
            }

            fprintf(temp_file, "%lu ", cur_len);
        }
        fprintf(temp_file, "\n");

        HTableRemove(htable);
        printf("%s loading completed\n", hash_funcs[n_func].name);
    }

    fclose(temp_file);

    printf("Initiating drawing script\n");
    system("python3 ../src/graphics.py");

    printf("Drawing successful\n");
    return;
}
//----------------------------------------------------------------------------------------//

void LoadHTable(const text_storage* text, const text_storage* dict, const size_t htable_size){
    
    assert(text != NULL);
    assert(dict != NULL);

    HTable* htable = NULL;
    HTableInit(&htable, htable_size);
    assert(htable != NULL);

    for(uint n_word = 0; n_word < dict->n_words; n_word++){
        HTableInsert(htable, GetWord(dict, n_elem), 0);
    }

    // TODO: норм юзкейс?
    for(uint n_word = 0; n_word < text->n_words; n_word++){

        uint cur_freq = 0;
        HTableFind(htable, GetWord(text, n_word), &cur_freq);
        HTableInsert(htable, cur_freq + 1);
    }

    return;
}
//----------------------------------------------------------------------------------------//

void InitTesting(){
    uint mode = 0;

    while(1){
        printf("Choose usage mode:\n"
        "1 - get spectral analyziz\n"
        "2 - make load of hash-table\n");

        scanf("%u", &mode);

        if(!(mode == LOAD || mode == SPECTRAL_ANALYSIS)){
            printf("unrecognized mode, try again\n");
        }
        else{
            break;
        }
    }

    char dict_file_name[256] = "";

    printf("Write name of dictionary file\n");
    scanf("%s", dict_file_name);

    text_storage* dict = GetStorage(dict_file_name);

    if(dict == NULL){
        printf("unable to open %s\n", data_file_name);
        return;
    }

    uint htable_size = 0;
    printf("Enter hash_table size\n");
    scanf("%u", &htable_size);

    if(htable_size == 0 || htable_size > MAX_HASH_SIZE){
        printf("hash_size must be in range [1, %u]\n", MAX_HASH_SIZE);
        return;
    }

    if(mode == LOAD){

        char data_file_name[256] = "";

        printf("Write name of file to get frequency statistic\n");
        scanf("%s", data_file_name);

        text_storage* text_data = GetStorage(data_file_name);
    
        if(text_data == NULL){
            printf("unable to open %s\n", data_file_name);
            return;
        }

        LoadHTable(text_data, dict, htable_size);
    }
    else if(mode == SPECTRAL_ANALYSIS){
        
        BAR_DRAW_MODE save_mode = BAR_DRAW_MODE::ONE_IMAGE;
        uint is_show = 0;

        char ans = 0;
        printf("Do u want to get multiple images for every function?(Y/N)\n");
        scanf("%c", &ans);

        if(ans == 'Y') save_mode = BAR_DRAW_MODE::MULTIPLE_IMAGES;

        printf("Do u want to see graphics?(Y/N)\n");
        scanf("%c", ans);

        if(ans == 'Y') is_show = 1;

        GetSpectralAnalysis(dict, htable_size, save_mode, is_show);
    }

    return;
}
