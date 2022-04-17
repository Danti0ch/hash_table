/**
 * \file
 * \brief  файл содержит определения функций, объявленных в text_storage.h
 *
 */

#include "text_storage.h"

//----------------------LOCAL-FUNCTIONS-DECLARATION-----------------------//


/**
 * получает количество строк и символов файла
 * 
 * \param file_name имя файла, размер которого нужно узнать
 * \param n_lines указатель на переменную, куда нужно записать количество строк в файле
 * \param len указатель на переменную, куда нужно записать количество символов в файле
 * 
 * \return код ошибки или успеха
 */
ERR_CODE get_file_metadata(const char *file_name, size_t *n_lines, size_t *len);

/**
 * @brief инициализирует text_storage
 * 
 * @param buf_size количество символов в буфере
 * @param n_lines количество строк в файле
 * @return text_storage* 
 */
text_storage* text_storage_init(const size_t buf_size, const size_t n_lines);

//----------------------PUBLIC-FUNCTIONS-DEFINITIONS----------------------//

text_storage* text_storage_init(const size_t buf_size, const size_t n_lines, const size_t n_words){

    if(buf_size == 0 || n_lines == 0 || n_words == 0) return NULL;
    
    text_storage* storage = (text_storage*)calloc(1, sizeof(text_storage));
    assert(storage != NULL);

    storage->len_buf = buf_size;
    storage->n_lines = n_lines;
    storage->n_words = n_words;

    storage->p_lines = (line_storage*)calloc(n_lines, sizeof(line_storage));
    storage->p_words = (string*)calloc(n_words, sizeof(string));
    storage->buffer  = (char*)calloc(buf_size, sizeof(char));

    assert(storage->p_lines != NULL && storage->buffer != NULL && storage->p_words != NULL);

    return storage;
}
//----------------------------------------------------------------------------------------//


// TODO: refactor acuired
text_storage* GetStorage(const char *file_name){

    assert(storage != NULL);
    assert(file_name != NULL);

    size_t n_lines = 0, buf_size = 0, n_words = 0;

    get_file_len(file_name, &n_lines, &buf_size, &n_words);

    if(getting_len_result != OK)    assert(0);
    if(storage->len_buf == 0)   assert(0);

    text_storage* storage = text_storage_init(buf_size, n_lines, n_words);

    if(mem_for_storage_result != OK)    assert(0);

    FILE *input_file = fopen(file_name, "r");

    assert(input_file != NULL);

    int reading_status = fread(storage->buffer, sizeof storage->buffer[0], storage->len_buf, input_file);
    assert(reading_status >= 0);

    fclose(input_file);

    storage->buffer[storage->len_buf - 1] = '\n';

    uint    n_line          = 0;
    uint    n_word          = 0;
    uint    n_symbs_in_line = 0;

    for(int ind_buf = 0; ind_buf < storage->len_buf; ind_buf++){

        // word handler
        if(!isspace(storage->buffer[ind_buf])){

            storage->p_words[n_word].pt = storage->buffer + ind_buf;

            uint cur_word_len = ind_buf;
            while(!isspace(storage->buffer[ind_buf])){
                ind_buf++;
            }
            cur_word_len = ind_buf - cur_word_len;
            
            storage->p_words[n_word].len = cur_word_len;
            n_word++;
        }

        // eol handler
        if(storage->buffer[ind_buf] == '\n'){

            storage->p_lines[n_line].p_line = storage->buffer + n_symbs_in_line;

            n_symbs_in_line = ind_buf - n_symbs_in_line + 1;
            storage->p_lines[n_line].len     = n_symbs_in_line;
            storage->p_lines[n_line].n_words = n_word;

            n_word = 0;
            n_symbs_in_line = ind_buf + 1;
            n_line++;
        }

        // space symb handler
        if(isspace(storage->buffer[ind_buf])){
            storage->buffer[ind_buf] = 0;
        }
    }

    return storage;
}
//----------------------------------------------------------------------------------------//

/*
ERR_CODE WriteStorage(FILE *output_file, const text_storage *storage){

    assert(storage     != NULL);
    assert(output_file != NULL);

    for(int i = 0; i < storage->n_lines; i++){
        fputs(storage->p_lines[i].p_line, output_file);
        fputc('\n', output_file);
    }
    return OK;
}
*/
//----------------------------------------------------------------------------------------//

ERR_CODE WriteBufferOfStorage(FILE *output_file, const text_storage *storage){

    assert(storage     != NULL);
    assert(output_file != NULL);

    char *n_symb = storage->buffer;

    for(int n_line = 0; n_line < storage->n_lines; n_line++, n_symb++){
        for(; *n_symb != '\0'; n_symb++){
            fputc(*n_symb, output_file);
        }
        fputc('\n', output_file);
    }

    return OK;
}
//----------------------------------------------------------------------------------------//

ERR_CODE TextStorageRemove(text_storage *storage){

    assert(storage != NULL);
    
    free(storage->p_lines);
    free(storage->buffer);
    free(storage->p_words);
    free(storage);

    return OK;
}
//----------------------------------------------------------------------------------------//

//----------------------LOCAL-FUNCTIONS-DEFINITIONS----------------------//

ERR_CODE get_file_metadata(const char *file_name, size_t *n_lines, size_t *len, size_t* n_words){

    assert(file_name != NULL);

    assert(n_lines != NULL);
    assert(len != NULL);
    assert(len != n_lines);
    
    *len     = 0;
    *n_lines = 0;
    *n_words = 0;

    FILE *text_file = fopen(file_name, "r");

    assert(text_file != NULL);
    
    fseek(text_file, 0, SEEK_END);

    size_t file_size = ftell(text_file);

    if(file_size <= 0){
        fclose(text_file);
        return EMPTY_FILE;
    }

    fseek(text_file, 0, SEEK_SET);

    char *buffer = (char*)calloc(file_size + 2, sizeof(char));

    size_t n_readen_bytes = fread(buffer, sizeof buffer[0], file_size + 1, text_file);

    fclose(text_file);
    assert(n_readen_bytes > 0);

    buffer[n_readen_bytes] = '\n';

    *len = n_readen_bytes;

    for(size_t n_byte = 0; n_byte < n_readen_bytes; n_byte++){
        if(buffer[n_byte] == '\n'){
            (*n_lines)++;
        }
        if(!isspace(buffer[n_byte]) && isspace(buffer[n_byte + 1])){
            (*n_words)++;
        }
    }

    // если на последней непустой строчке нет символа переноса
    if((buffer[n_readen_bytes - 1] > 0) && (buffer[n_readen_bytes - 1] != '\n')){
        (*n_lines)++;
    }

    free(buffer);

    return OK;
}
//----------------------------------------------------------------------------------------//
