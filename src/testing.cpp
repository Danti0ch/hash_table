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

    fprintf(temp_file, "%d %u\n", save_mode, is_show);
    fprintf(temp_file, "%lu %lu %u %u\n", data->n_words, htable_size, N_HASH_FUNCS/2, N_HASH_FUNCS/2);

    for(uint n_func = 0; n_func < N_HASH_FUNCS; n_func++){

        printf("Insertion of %s\n", hash_funcs[n_func].name);

        fprintf(temp_file, "%s\n", hash_funcs[n_func].name);
        fprintf(temp_file, "%s\n", hash_funcs[n_func].descr);

        HTable* htable = HTableInitCustomHash(htable_size, hash_funcs[n_func].p_func);
        
        assert(htable != NULL);

        for(uint n_elem = 0; n_elem < data->n_words; n_elem++){
            HTableInsert(htable, GetWord(data, n_elem), 0);
        }
        fprintf(temp_file, "%lu\n", htable->n_init_lists);

        size_t htable_size = htable->n_lists;
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

        size_t cur_len = 0;
        for(uint n_list = 0; n_list < htable_size; n_list++){

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

    HTable* htable = HTableInit(htable_size);
    assert(htable != NULL);

    for(uint n_word = 0; n_word < dict->n_words; n_word++){
        if(n_word % 100000  == 0) printf("pack %u loaded\n", n_word);
        HTableInsert(htable, GetWord(dict, n_word), 0);
    }

    // TODO: норм юзкейс?

    uint n_recognized = 0;

    for(uint n_word = 0; n_word < text->n_words; n_word++){

        int cur_freq = 0;
        const char* cur_word = GetWord(text, n_word);

        int isfind = HTableFind(htable, cur_word, &cur_freq);
        if(isfind){
            n_recognized++;
            HTableInsert(htable, cur_word, cur_freq + 1);
        }

        if(n_word % 1000000  == 0){
            printf("%u/%lu recognized\n", n_recognized, text->n_words);
        }
    }

    HTableRemove(htable);
    return;
}
//----------------------------------------------------------------------------------------//

void InitTesting(){
    uint mode = 0;

    while(1){
        printf("Choose usage mode:\n"
        "%d - get spectral analyziz\n"
        "%d - make load of hash-table\n", SPECTRAL_ANALYSIS, LOAD);

        scanf("%u", &mode);

        if(!(mode == LOAD || mode == SPECTRAL_ANALYSIS)){
            printf("unrecognized mode, try again\n");
        }
        else{
            break;
        }
    }

    char dict_file_name[256] = "";

    text_storage* dict = NULL;

    while(dict == NULL){
        printf("Write name of dictionary file\n");
        scanf("%s", dict_file_name);

        if(dict_file_name[0] == '0'){
            strcpy(dict_file_name, DEFAULT_DICT_FILE_NAME);
        }

        dict = GetStorage(dict_file_name);

        if(dict == NULL){
            printf("unable to open %s, try again\n", dict_file_name);
        }
    }

    uint htable_size = 0;
    printf("Enter hash_table size\n");
    scanf("%u", &htable_size);
    if(htable_size == 0) htable_size = DEFAULT_HT_SIZE;

    if(htable_size == 0 || htable_size > MAX_HASH_SIZE){
        printf("hash_size must be in range [1, %u]\n", MAX_HASH_SIZE);
        return;
    }

    if(mode == LOAD){

        char data_file_name[256] = "";

        printf("Write name of file to get frequency statistic\n");
        scanf("%s", data_file_name);

        if(data_file_name[0] == '0'){
            strcpy(data_file_name, DEFAULT_FREQ_STAT_FILE_NAME);
        }

        text_storage* text_data = NULL;

        while(text_data == NULL){
            
            text_data = GetStorage(data_file_name);
            if(text_data == NULL){
                printf("unable to open %s\n", data_file_name);
            }
        }   
    
        if(text_data == NULL){
            printf("unable to open %s\n", data_file_name);
            return;
        }

        uint n_tests = 16;
        printf("write number of tests: ");
        scanf("%u", &n_tests);

        if(n_tests == 0) n_tests = DEFAULT_TESTS_NUMBER;

        uint start_time = clock();   

        for(uint n_test = 0; n_test < n_tests; n_test++){
            LoadHTable(text_data, dict, htable_size);
        }

        uint delt = (uint)clock() - start_time;
        printf("load time: %g secs\n", ((double)delt)/(((double)CLOCKS_PER_SEC)*((double)n_tests)));

    }
    else if(mode == SPECTRAL_ANALYSIS){
        
        BAR_DRAW_MODE save_mode = BAR_DRAW_MODE::ONE_IMAGE;
        uint is_show = 0;

        char ans = 0;
        printf("Do u want to get multiple images for every function?(Y/N)\n");
        scanf("%c", &ans);

        if(ans == 'Y') save_mode = BAR_DRAW_MODE::MULTIPLE_IMAGES;

        printf("Do u want to see graphics?(Y/N)\n");
        scanf("%c", &ans);

        if(ans == 'Y') is_show = 1;

        GetSpectralAnalysis(dict, htable_size, save_mode, is_show);
    }

    return;
}
