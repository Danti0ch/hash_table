#include "hash_table.h"
#include <stdlib.h>
#include "logger.h"
#include <assert.h>
#include <nmmintrin.h>
#include <math.h>
#include <string.h>

// TODO: readme

const int INCREASE_RATIO = 1;

// режим для resize
const int INCREASE_MODE  = 1;
const int REDUCE_MODE    = 0;

// TODO: обработка ошибок в list
#if HT_TOTAL_DUMP

    #define HTABLE_OK(obj)                                               \
    {                                                                     \
        HT_VERIF_CODE ver_code = verify_htable(obj);                       \
        meta_info meta = {};                                                \
                                                                            \
        meta.obj_name = (char*)calloc(strlen(obj_name), sizeof(char));      \
        strcpy(meta.obj_name, obj_name);                                    \
                                                                            \
        meta.file_name = (char*)calloc(strlen(file_name), sizeof(char));    \
        strcpy(meta.file_name, file_name);                                  \
                                                                            \
        meta.func_name = (char*)calloc(strlen(func_name), sizeof(char));    \
        strcpy(meta.func_name, func_name);                                  \
                                                                            \
        meta.n_line    = (uint)n_line;                                      \
        meta.ver_code  = (uint)ver_code;                                    \
                                                                            \
        htable_dump((obj), &(meta), #obj, LOCATION);                        \
                                                                            \
        free(meta.obj_name);                                                \
        free(meta.file_name);                                               \
        free(meta.func_name);                                               \
                                                                            \
        if(ver_code  != HT_VERIF_CODE::OK){                                 \
            return HT_ERR_CODE::VERIFY_FAILED;                              \
        }                                                                   \
    }

#elif HT_ERR_DUMP

    #define HTABLE_OK(obj)                                                      \
    {                                                                            \
        HT_VERIF_CODE ver_code = verify_htable(obj);                              \
        if(ver_code  != HT_VERIF_CODE::OK){                                        \
                meta_info meta = {};                                                \
                                                                                    \
                meta.obj_name = (char*)calloc(strlen(obj_name), sizeof(char));      \
                strcpy(meta.obj_name, obj_name);                                    \
                                                                                    \
                meta.file_name = (char*)calloc(strlen(file_name), sizeof(char));    \
                strcpy(meta.file_name, file_name);                                  \
                                                                                    \
                meta.func_name = (char*)calloc(strlen(func_name), sizeof(char));    \
                strcpy(meta.func_name, func_name);                                  \
                                                                                    \
                meta.n_line    = (uint)n_line;                                      \
                meta.ver_code  = (uint)ver_code;                                    \
                                                                                    \
                htable_dump((obj), &(meta), #obj, LOCATION);                        \
                                                                                    \
                free(meta.obj_name);                                                \
                free(meta.file_name);                                               \
                free(meta.func_name);                                               \
                                                                                    \
                return HT_ERR_CODE::VERIFY_FAILED;                                  \
        }                                                                           \
    }

#else

    #define HTABLE_OK(obj)                            \
    {                                                  \
        HT_VERIF_CODE ver_code = verify_htable(obj);    \
                                                        \
        if(ver_code  != HT_VERIF_CODE::OK){             \
            return HT_ERR_CODE::VERIFY_FAILED;          \
        }                                               \
    }

#endif //HT_TOTAL_DUMP
//========================================================================================//

//                          LOCAL_FUNCTIONS_DECLARATION

//========================================================================================//

HT_VERIF_CODE        verify_htable(const HTable* obj);
static void          htable_dump(const HTable* obj, meta_info* meta, META_PARAMS);
static uint          fill_factor_excess(double fill_factor);
HT_ERR_CODE          resize(HTable* obj);

/// @brief переливание данных из dest в src
HT_ERR_CODE          transfuse_data(HTable* dest, const HashTable* src);
static void          htable_swap(HTable* obj1, HTable* obj2);

/// @brief извлекает свободную позицию в буфере хэш таблицы
static uint          extract_free_place(HTable* obj);
static void          resize_buffer(HTable* obj);

uint get_hash(const char* str);

//========================================================================================//

//

//                          PUBLIC_FUNCTIONS_DEFINITION

//

//========================================================================================//

// error handling
HTable* _HTableInit(const size_t size, LOC_PARAMS){

    if(size == 0) return NULL;

    HTable* obj = (HTable*)calloc(1, sizeof(HTable));
    if(obj == NULL) return NULL;

    obj->data = (list**)calloc(size, sizeof(list*));
    
    if(obj->data == NULL){
        free(obj);
        return NULL;
    }

    obj->buffer = (char*)aligned_alloc(ALIGN_RATIO, ALIGN_RATIO * size * sizeof(char));
    if((*obj)->buffer == NULL){
        free(*obj);
        free((*obj)->data);
        return HT_ERR_CODE::UNABLE_ALLOC_MEM;
    }

    memset((*obj)->buffer, 0, size);

    (*obj)->buf_free_vals = (uint*)calloc(size, sizeof(uint));
    if((*obj)->buf_free_vals == NULL){
        free(*obj);
        free((*obj)->data);
        free((*obj)->buffer);
        return HT_ERR_CODE::UNABLE_ALLOC_MEM;
    }

    (*obj)->size            = size;
    (*obj)->n_elems         = 0;
    (*obj)->hash_func       = get_hash;
    (*obj)->fill_factor     = 0;
    (*obj)->total_mem_size  = 0;
    (*obj)->n_init_lists    = 0;
    (*obj)->n_words         = size;

    for(uint n_list = 0; n_list < size; n_list++){
        (*obj)->data[n_list] = LST_POISON;
    }
    for(uint n_word = 0; n_word < size; n_word++){
        (*obj)->buf_free_vals[n_word] = n_word + 1;
    }
    (*obj)->buf_free_vals[size - 1] = -1;

    return HT_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

HT_ERR_CODE _HTableInitCustomHash(HTable** obj, const size_t size, uint (*p_func)(const char* str), LOC_PARAMS){
    
    assert(obj != NULL);

    if(*obj  != NULL) return HT_ERR_CODE::INVALID_FREE_TABLE;
    if(size == 0)    return HT_ERR_CODE::INVALID_SIZE;

    *obj = (HTable*)calloc(1, sizeof(HTable));
    if(*obj == NULL) return HT_ERR_CODE::UNABLE_ALLOC_MEM;

    (*obj)->data = (list**)calloc(size, sizeof(list*));
    
    if((*obj)->data == NULL){
        free(*obj);
        return HT_ERR_CODE::UNABLE_ALLOC_MEM;
    }

    (*obj)->buffer = (char*)aligned_alloc(ALIGN_RATIO, ALIGN_RATIO * size * sizeof(char));
    if((*obj)->buffer == NULL){
        free(*obj);
        free((*obj)->data);
        return HT_ERR_CODE::UNABLE_ALLOC_MEM;
    }
    memset((*obj)->buffer, 0, size);

    (*obj)->buf_free_vals = (uint*)calloc(size, sizeof(uint));
    if((*obj)->buf_free_vals == NULL){
        free(*obj);
        free((*obj)->data);
        free((*obj)->buffer);
        return HT_ERR_CODE::UNABLE_ALLOC_MEM;
    }

    (*obj)->size            = size;
    (*obj)->n_elems         = 0;
    (*obj)->hash_func       = p_func;
    (*obj)->fill_factor     = 0;
    (*obj)->total_mem_size  = 0;
    (*obj)->n_init_lists    = 0;
    (*obj)->n_words         = size;

    for(uint n_list = 0; n_list < size; n_list++){
        (*obj)->data[n_list] = LST_POISON;
    }
    for(uint n_word = 0; n_word < size; n_word++){
        (*obj)->buf_free_vals[n_word] = n_word + 1;
    }
    (*obj)->buf_free_vals[size - 1] = -1;

    return HT_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

HT_ERR_CODE _HTableFind(const HTable* obj, const list_T str, uint* is_in_table, META_PARAMS){

    HTABLE_OK(obj)

    assert(obj         != NULL);
    assert(str         != NULL);
    assert(is_in_table != NULL);
    
    uint list_ind = get_hash(str) % obj->size;

    list* cur_list = obj->data[list_ind];

    if(cur_list == LST_POISON){
        *is_in_table = 0;
        return HT_ERR_CODE::OK;
    }

    node* res = ListFind(cur_list, str);

    if(res == NULL)     *is_in_table = 0;
    else                *is_in_table = 1;

    HTABLE_OK(obj)

    return HT_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

HT_ERR_CODE _HTableInsert(HTable* obj, const list_T str, META_PARAMS){

    assert(obj != NULL);
    HTABLE_OK(obj)

    uint list_ind = obj->hash_func(str) % obj->size;

    assert(list_ind < obj->size);

    if(obj->data[list_ind] == LST_POISON){
        obj->data[list_ind]  = ListConstructor(1);
        obj->n_init_lists++;
    }
    else{
        node* res = ListFind(obj->data[list_ind], str);

        if(res != NULL) return HT_ERR_CODE::OK;
    }

    uint position = extract_free_place(obj);
    
    strcpy(obj->buffer + position, str);

    PushBack(obj->data[list_ind], obj->buffer + position);

    obj->n_elems++;

    #if RESIZE_ENABLE
        obj->fill_factor = (double)obj->n_elems / (double)obj->size;
        
        if(fill_factor_excess(obj->fill_factor)){
            HT_ERR_CODE res = resize(obj);
            if(res != HT_ERR_CODE::OK) return res;
        }
    #endif //   RESIZE_ENABLE
    HTABLE_OK(obj)
    return HT_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

void _HTableRemove(HTable* obj, META_PARAMS){

    if(obj == NULL) return;

    for(uint n_list = 0; n_list < obj->size; n_list++){
        if(obj->data[n_list] != LST_POISON){
            ListDestructor(obj->data[n_list]);
        }
    }

    free(obj->buf_free_vals);
    free(obj->buffer);
    free(obj->data);
    free(obj);

    return;
}
//----------------------------------------------------------------------------------------//

HT_ERR_CODE _HTableGetSize(HTable* obj, size_t* dest, META_PARAMS){

    assert(obj  != NULL);
    assert(dest != NULL);
    HTABLE_OK(obj)

    *dest = obj->size;

    return HT_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//
uint GetListSize(const HTable* obj, const uint n_list);

//========================================================================================//

//

//                          LOCAL_FUNCTIONS_DEFINITION

//

//========================================================================================//

HT_VERIF_CODE verify_htable(const HTable* obj){

    assert(obj != NULL);

    if(obj->data == NULL) return HT_VERIF_CODE::EMPTY_MEM;
    if(obj->size == 0)    return HT_VERIF_CODE::SIZE_CORRUPTED;

    if(obj->hash_func == NULL)  return HT_VERIF_CODE::HASH_FUNC_CORRUPTED;

    if(obj->n_init_lists > obj->n_elems) return HT_VERIF_CODE::NLISTS_OVERFLOW;

#if HASH_CHECK
    for(uint n_list = 0; n_list < obj->size; n_list++){

        list* cur_list = obj->data[n_list];

        LIST_VERIF_CODE ver_code = VerifyList(cur_list);

        if(ver_code != LIST_VERIF_CODE::OK){
            return LIST_VERIF_CODE::LIST_VERIFY_FAILED;
        }

        for(uint n_elem = 0; n_elem < cur_list->size; n_elem++){
            
            uint hash_val = get_hash(cur_list->nodes[n_elem].val);
            if(hash_val != n_list){
                return LIST_VERIF_CODE::HASH_VALUE_INVALID;
            }
        }
    }

#elif LIST_CHECK
    for(uint n_list = 0; n_list < obj->size; n_list++){

        list* cur_list = obj->data[n_list];

        LIST_VERIF_CODE ver_code = VerifyList(cur_list);

        if(ver_code != LIST_VERIF_CODE::OK){
            return LIST_VERIF_CODE::LIST_VERIFY_FAILED;
        }
    }
#endif //HASH_CHECK

    return HT_VERIF_CODE::OK;
}
//----------------------------------------------------------------------------------------//

static void htable_dump(const HTable* obj, meta_info* meta, META_PARAMS){

    assert(obj  != NULL);
    assert(meta != NULL);

    LOG("<br><br>Dump called from function %s(%d)<br>"
        "That was called from file %s, function %s(%d)<br>"
        "[#] hash table %s<int><br>"
        "<br>"
        "size            = %lu<br>"
        "n_elems         = %lu<br>"
        "ptr data        = %p<br>"
        "total_mem_size  = %lu<br>"
        "n_init_lists    = %lu<br>"
        "<br>",
        func_name, n_line, meta->file_name, meta->func_name, meta->n_line,
        meta->obj_name,
        obj->size, obj->n_elems, obj->data, obj->total_mem_size, obj->n_init_lists);

    LOG("<br>");

    for(uint n_list = 0; n_list < obj->size; n_list++){

        if(obj->data[n_list] != LST_POISON){
            LOG("<br>List: %u: <br>", n_list);
            #if LIST_FULL_INFO_DUMP
                ListDump(obj->data[n_list], meta, META_PARAMS);
            #else
                DumpNodes(obj->data[n_list]);
            #endif // LIST_FULL_INFO_DUMP
        }
    }

    LOG("<br><br>=====================================================================================<br><br>");

    return;
}
//----------------------------------------------------------------------------------------//


static uint fill_factor_excess(double fill_factor){
    if(fill_factor < FILL_FACTOR_LIMIT){
        return 0;
    }

    return 1;
}
//----------------------------------------------------------------------------------------//

HT_ERR_CODE resize(HTable* obj){

    assert(obj  != NULL);

    size_t new_size = obj->size << INCREASE_RATIO;
    
    HTable* new_obj = NULL;

    HT_ERR_CODE init_res = HTableInitCustomHash(&new_obj, new_size, obj->hash_func);

    if(init_res != HT_ERR_CODE::OK) return init_res;

    transfuse_data(new_obj, obj);

    htable_swap(new_obj, obj);

    HTableRemove(new_obj);

    return HT_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

HT_ERR_CODE transfuse_data(HTable* dest, const HashTable* src){
    
    assert(dest != NULL);
    assert(src  != NULL);
    assert(dest != src);

    for(uint n_list = 0; n_list < src->size; n_list++){

        list* cur_list = src->data[n_list];

        if(cur_list != LST_POISON){
            for(uint n_elem = 0; n_elem < cur_list->size; n_elem++){
                HT_ERR_CODE res = HTableInsert(dest, cur_list->nodes[n_elem].val);
                if(res != HT_ERR_CODE::OK) return res;
            }
        }
    }

    return HT_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

// TODO: xor swap rebuild
static void htable_swap(HTable* obj1, HTable* obj2){
    
    assert(obj1 != NULL);
    assert(obj2 != NULL);

    HTable temp = *obj1;
    *obj1       = *obj2;
    *obj2       = temp;

    return;
}
//----------------------------------------------------------------------------------------//

static void resize_buffer(HTable* obj){

    assert(obj != NULL);

    size_t new_size = ALIGN_RATIO * obj->n_words * 2;
    char* new_buffer = (char*)aligned_alloc(ALIGN_RATIO, new_size);
    assert(new_buffer != NULL);

    memset(new_buffer, 0, new_size);
    for(uint n_list = 0; n_list < obj->size; n_list++){

        list* cur_list = obj->data[n_list];
        if(cur_list == LST_POISON) continue;

        size_t list_size = cur_list->size;

        for(uint n_word = 0; n_word < list_size; n_word++){
            
            cur_list->nodes[n_word].val = new_buffer + (uint)((char*)cur_list->nodes[n_word].val - obj->buffer);
        }
    }

    memcpy(new_buffer, obj->buffer, obj->n_words * ALIGN_RATIO);

    uint* new_free_vals = (uint*)calloc(obj->n_words * 2, sizeof(uint));
    assert(new_free_vals != NULL);

    memcpy(new_free_vals, obj->buf_free_vals, obj->n_words);

    size_t n_words = obj->n_words;
    for(uint i = n_words; i < n_words * 2; i++){
        new_free_vals[i] = i+1;
    }
    new_free_vals[n_words * 2 - 1] = -1;

    if(obj->free_head == -1){
        obj->free_head = obj->n_words;
    }
    else{
        obj->buf_free_vals[obj->free_tail] = obj->n_words;
    }
    obj->free_tail = obj->n_words * 2 - 1;

    free(obj->buffer);
    free(obj->buf_free_vals);

    obj->buffer = new_buffer;
    obj->buf_free_vals = new_free_vals;

    return;
}
//----------------------------------------------------------------------------------------//

static uint extract_free_place(HTable* obj){

    assert(obj != NULL);

    if(obj->free_head == -1){
        resize_buffer(obj);
    }
    if(obj->buf_free_vals[obj->free_head] == -1){
        resize_buffer(obj);
    }

    uint n_elem = obj->free_head;

    uint next_elem_ind = obj->buf_free_vals[obj->free_head];
    obj->buf_free_vals[obj->free_head] = -1;
    obj->free_head = next_elem_ind;

    return n_elem * ALIGN_RATIO;
}
//----------------------------------------------------------------------------------------//

/*
static void remove_data(HTable* obj){

    assert(obj != NULL);

    if(obj->data == NULL) return;

    for(uint n_list = 0; n_list < obj->size; n_list++){
        if(obj->data[n_list] != LST_POISON){
            ListDestructor(obj->data[n_list]);
        }
    }

    free(obj->data);

    return;
}
//----------------------------------------------------------------------------------------//
*/

uint get_hash(const char* str){
    
    assert(str != NULL);

    #if OPTIMIZE_DISABLE
        uint hash = 0xFFFFFFFF;

        for(uint i = 0; str[i] != 0; i++){

            hash = (hash << 8) ^ crc32_table[((hash >> 24) ^ str[i]) & 0xFF];
        }
    #else

        uint hash = 0xFFFFFFFF;

        for(uint ind = 0; ind < 32; ind += 8){
            uint hash_val = *((unsigned long long*)(str + ind));
            if(hash_val == 0) break;
            hash = _mm_crc32_u64(hash, hash_val);
        }
            
    #endif  // OPTIMIZE_DISABLE

    return hash;
}   
//----------------------------------------------------------------------------------------//
