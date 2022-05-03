/**
 * \file
 * \brief  файл содержит определения функций, объявленных в text_storage.h
 *
 */

#include "text_storage.h"
#include <ctype.h>
#include "list.h"

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
err_code get_file_metadata(const char *file_name, size_t *n_lines, size_t *len, size_t* n_words);

void get_buffer_metadata(const char* buffer, const size_t buf_size, size_t* n_lines, size_t* n_words);

/**
 * @brief инициализирует text_storage
 * 
 * @param buf_size количество символов в буфере
 * @param n_lines количество строк в файле
 * @return text_storage* 
 */
text_storage* text_storage_init(const size_t buf_size, const size_t n_lines);

int word_cmp(const word* w1, const word* w2);

static char* generate_buffer(const word* p_words, const size_t n_words, size_t* buf_size);

void construct_auxil_arrays(text_storage* storage);

static void align_words(text_storage* storage);

size_t get_align_buf_size(text_storage* storage);


//----------------------PUBLIC-FUNCTIONS-DEFINITIONS----------------------//

text_storage* text_storage_init(const size_t buf_size, const size_t n_lines, const size_t n_words){

    if(buf_size == 0 || n_lines == 0 || n_words == 0) return NULL;
    
    text_storage* storage = (text_storage*)calloc(1, sizeof(text_storage));
    assert(storage != NULL);

    storage->len_buf = buf_size;
    storage->n_lines = n_lines;
    storage->n_words = n_words;

    storage->p_lines = (line_storage*)calloc(n_lines, sizeof(line_storage));
    storage->p_words = (word*)calloc(n_words, sizeof(word));
    storage->buffer  = (char*)calloc(buf_size, sizeof(char));

    assert(storage->p_lines != NULL && storage->buffer != NULL && storage->p_words != NULL);

    return storage;
}
//----------------------------------------------------------------------------------------//

// TODO: refactor acuired
text_storage* GetStorage(const char *file_name){

    assert(file_name != NULL);

    size_t n_lines = 0, buf_size = 0, n_words = 0;

    

    get_file_metadata(file_name, &n_lines, &buf_size, &n_words);

    text_storage* storage = text_storage_init(buf_size, n_lines, n_words);
    

    //if(mem_for_storage_result != OK)    assert(0);

    FILE *input_file = fopen(file_name, "r");
    assert(input_file != NULL);

    int reading_status = fread(storage->buffer, sizeof storage->buffer[0], storage->len_buf, input_file);
    assert(reading_status >= 0);

    fclose(input_file);

    construct_auxil_arrays(storage);
    
    align_words(storage);

    return storage;
}
//----------------------------------------------------------------------------------------//

text_storage* GetStorage(const char *buffer, const size_t buf_size){

    assert(buffer != NULL);

    size_t n_lines = 0, n_words = 0;

    // TODO:
    get_buffer_metadata(buffer, buf_size, &n_lines, &n_words);

    text_storage* storage = text_storage_init(buf_size, n_lines, n_words);

    //if(mem_for_storage_result != OK)    assert(0);

    memcpy(storage->buffer, buffer, buf_size * sizeof(char));

    // TODO: rename ???
    construct_auxil_arrays(storage);    

    return storage;
}
//----------------------------------------------------------------------------------------//

text_storage* GetStorage(const text_storage* src_storage){

    assert(src_storage != NULL);

    text_storage* dest_storage = text_storage_init(src_storage->len_buf, src_storage->n_lines, src_storage->n_words);

    memcpy(dest_storage->buffer,   src_storage->buffer,   src_storage->len_buf * sizeof(char));
    memcpy(dest_storage->p_words,  src_storage->p_words,  src_storage->n_words * sizeof(word));
    memcpy(dest_storage->p_lines,  src_storage->p_lines,  src_storage->n_lines * sizeof(line_storage));
    
    return dest_storage;
}
//----------------------------------------------------------------------------------------//

// TODO: нужно переделать, так как рушится инкапсуляция(words указывает на слова из какого-то буфера)

void MakeUniqueData(text_storage* storage){

    assert(storage != NULL);

    size_t n_words = storage->n_words;

    word* uniq_words = (word*)calloc(n_words, sizeof(word));
 
    memcpy(uniq_words, storage->p_words, n_words * sizeof(word));
 
    qsort(uniq_words, n_words, sizeof(word), (int(*) (const void *, const void *))word_cmp);

    memcpy(uniq_words, uniq_words, sizeof(word));
    uint cur_word = 1;

    for(uint i = 1; i < n_words; i++){

        if(word_cmp(uniq_words + i, uniq_words + i - 1) != 0){            
            memcpy(uniq_words + cur_word, uniq_words + i, sizeof(word));
            cur_word++;
        }
    }

    size_t buf_size = 0;
    char* uniq_buffer = generate_buffer(uniq_words, cur_word, &buf_size);

    free(uniq_words);

    free(storage->buffer);
    free(storage->p_words);
    free(storage->p_lines);

    storage->buffer  = uniq_buffer;

    storage->len_buf = buf_size;
    storage->n_lines = 1;
    storage->n_words = cur_word;

    storage->p_lines = (line_storage*)calloc(1, sizeof(line_storage));
    storage->p_words = (word*)calloc(cur_word, sizeof(word));

    construct_auxil_arrays(storage);
    return;
}
//----------------------------------------------------------------------------------------//

err_code WriteBufferOfStorage(FILE *output_file, const text_storage *storage){

    assert(storage     != NULL);
    assert(output_file != NULL);

    char *n_symb = storage->buffer;

    for(int n_line = 0; n_line < storage->n_lines; n_line++, n_symb++){
        for(; *n_symb != '\0'; n_symb++){
            fputc(*n_symb, output_file);
        }
        fputc('\n', output_file);
    }

    return R;
}
//----------------------------------------------------------------------------------------//

void WriteWords(const text_storage* storage, const char* file_name){

    assert(file_name != NULL);
    assert(storage   != NULL);

    FILE* output_file = fopen(file_name, "w");

    for(uint n_word = 0; n_word < storage->n_words; n_word++){
        fprintf(output_file, "%s\n", storage->p_words[n_word].pt);
    }

    fclose(output_file);

    return;
}
//----------------------------------------------------------------------------------------//

err_code TextStorageRemove(text_storage *storage){

    assert(storage != NULL);
    
    free(storage->p_lines);
    free(storage->buffer);
    free(storage->p_words);
    free(storage);

    return R;
}
//----------------------------------------------------------------------------------------//

//----------------------LOCAL-FUNCTIONS-DEFINITIONS----------------------//

err_code get_file_metadata(const char *file_name, size_t *n_lines, size_t *len, size_t* n_words){

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

    return R;
}
//----------------------------------------------------------------------------------------//

#define min(a, b) ((a) > (b)) ? (b) : (a)

int word_cmp(const word* w1, const word* w2){

    assert(w1 != NULL);
    assert(w2 != NULL);

    size_t min_size = min(w1->len, w2->len);

    for(uint i = 0; i < min_size && w1->pt[i] != 0 && w2->pt[i] != 0; i++){
        if(w1->pt[i] > w2->pt[i]){
            return 1;
        }
        else if(w1->pt[i] < w2->pt[i]){
            return -1;
        }
    }

    if(w1->len > w2->len)      return  1;
    else if(w1->len < w2->len) return -1;
    return 0;
}
//----------------------------------------------------------------------------------------//

static char* generate_buffer(const word* p_words, const size_t n_words, size_t* buf_size){

    assert(p_words  != NULL);
    assert(buf_size != NULL);

    *buf_size = 0;

    for(uint i = 0; i < n_words; i++){
        *buf_size += p_words[i].len + 1;     // +1 for the '\0'
    }

    char* buffer = (char*)calloc(*buf_size, sizeof(char));
    uint cur_buf_pos = 0;

    // TODO: ' ' issue
    for(uint i = 0; i < n_words; i++, cur_buf_pos += p_words[i - 1].len + 1){
        memcpy(buffer + cur_buf_pos, p_words[i].pt, p_words[i].len * sizeof(char));
        buffer[cur_buf_pos + p_words[i].len] = ' ';
    }

    buffer[*buf_size - 1] = 0;
    return buffer;
}
//----------------------------------------------------------------------------------------//

void construct_auxil_arrays(text_storage* storage){

    assert(storage != NULL);

    storage->buffer[storage->len_buf - 1] = '\n';

    uint    n_lines         = storage->n_lines;
    uint    n_line          = 0;
    uint    n_word          = 0;
    uint    n_symbs_in_line = 0;
    uint    n_words_in_line = 0;

    storage->p_lines[n_line].p_words = storage->p_words;
    storage->p_lines[n_line].p_line  = storage->buffer;

    for(int ind_buf = 0; ind_buf < storage->len_buf; ind_buf++){

        // word handler
        if(!isspace(storage->buffer[ind_buf]) && storage->buffer[ind_buf] != 0){

            storage->p_words[n_word].pt = storage->buffer + ind_buf;
            
            uint cur_word_len = ind_buf;
            while(!isspace(storage->buffer[ind_buf]) && storage->buffer[ind_buf] != 0){
                ind_buf++;
            }
            cur_word_len = ind_buf - cur_word_len;
            
            storage->p_words[n_word].len = cur_word_len;

            n_word++;
        }

        // eol handler
        if(storage->buffer[ind_buf] == '\n' || storage->buffer[ind_buf] == 0){

            storage->p_lines[n_line].p_line = storage->buffer + n_symbs_in_line;

            n_symbs_in_line = ind_buf - n_symbs_in_line + 1;
            storage->p_lines[n_line].len     = n_symbs_in_line;
            storage->p_lines[n_line].n_words = n_word - n_words_in_line;

            n_words_in_line = n_word;
            n_symbs_in_line = ind_buf + 1;
            n_line++;

            if(n_line < n_lines){
                storage->p_lines[n_line].p_words = storage->p_words + n_word;
                storage->p_lines[n_line].p_line  = storage->buffer  + ind_buf + 1;
            }
        }

        // space symb handler
        if(isspace(storage->buffer[ind_buf])){
            storage->buffer[ind_buf] = 0;
        }
    }

    return;
}
//----------------------------------------------------------------------------------------//

void get_buffer_metadata(const char* buffer, const size_t buf_size, size_t* n_lines, size_t* n_words){

    assert(buffer  != NULL);
    assert(n_lines != NULL);
    assert(n_words != NULL);

    *n_lines = 0;
    *n_words = 0;

    for(size_t n_byte = 0; n_byte < buf_size - 1; n_byte++){
        if(buffer[n_byte] == '\n'){
            (*n_lines)++;
        }
        if(!isspace(buffer[n_byte]) && isspace(buffer[n_byte + 1])){
            (*n_words)++;
        }
    }

    if(!isspace(buffer[buf_size - 1])){
        (*n_words)++;
    }

    // если на последней непустой строчке нет символа переноса
    if((buffer[buf_size - 1] > 0) && (buffer[buf_size - 1] != '\n')){
        (*n_lines)++;
    }

    return;
}
//----------------------------------------------------------------------------------------//

// TODO: line len fix
static void align_words(text_storage* storage){

    assert(storage != NULL);

    size_t new_len_buf = get_align_buf_size(storage);

    char* align_buffer  = (char*)aligned_alloc(ALIGN_RATIO, new_len_buf * sizeof(char));
    
    memset(align_buffer, 0, new_len_buf * sizeof(char));
    
    char* p_align       = align_buffer;
    uint n_lines        = storage->n_lines;

    uint total_nwords = 0;

    uint total_w = 0;

    for(uint n_line = 0; n_line < n_lines; n_line++){

        line_storage* cur_line = storage->p_lines + n_line;

        cur_line->p_line    = p_align;

        uint n_words        = cur_line->n_words;
        uint total_line_nwords = 0;
        
        // TODO: не обновляем len
        for(uint n_word = 0; n_word < n_words; n_word++){
            
            size_t cur_len = cur_line->p_words[n_word].len;
            
            // TODO: fix
            // если длина слова больше 32, то мы его не обрабатываем
            if(cur_len + 1 >= ALIGN_RATIO){
                continue;
            }

            strncpy(p_align, cur_line->p_words[n_word].pt, cur_len);

            uint offset = 0;
            if((cur_len + 1) % ALIGN_RATIO != 0){
                offset = ALIGN_RATIO - ((cur_len + 1) % ALIGN_RATIO);
            }

            storage->p_words[total_w].pt  = p_align;
            storage->p_words[total_w].len = cur_len;

            p_align += offset + cur_len + 1;
            total_line_nwords++;
            total_w++;
        }

        cur_line->p_words   = storage->p_words + total_nwords;

        total_nwords+= total_line_nwords;
        storage->p_lines[n_line].n_words = total_line_nwords;
    }

    //free(storage->buffer);

    storage->buffer     = align_buffer;
    storage->len_buf    = new_len_buf;
    storage->n_words    = total_nwords;

    return;
}
///----------------------------------------------------------------------------------------//

size_t get_align_buf_size(text_storage* storage){

    assert(storage != NULL);

    uint n_words = storage->n_words;

    size_t len = 0;

    for(uint n_word = 0; n_word < n_words; n_word++){
            
        size_t  cur_len     = storage->p_words[n_word].len;
        uint    offset      = 0;
        if((cur_len + 1) % ALIGN_RATIO != 0){
                offset = ALIGN_RATIO - ((cur_len + 1) % ALIGN_RATIO);
        }
        
        len += cur_len + 1 + (size_t)offset;
    }

    return len;
}
//----------------------------------------------------------------------------------------//
