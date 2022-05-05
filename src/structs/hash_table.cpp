#include "hash_table.h"
#include <stdlib.h>
#include "../logger.h"
#include <assert.h>
#include <nmmintrin.h>
#include <math.h>
#include <string.h>

// TODO: readme

const int INCREASE_RATIO = 1;

// TODO: FIX
list* LST_POISON = (list*)123;

#if OPTIMIZE_ENABLE
	static char* temp = (char*)aligned_alloc(ALIGN_RATIO, ALIGN_RATIO * sizeof(char));
#endif

// режим для resize
const int INCREASE_MODE  = 1;
const int REDUCE_MODE    = 0;

// TODO: обработка ошибок в list
#if HT_TOTAL_DUMP

    #define HTABLE_OK(obj, failed_red)                                   \
    {                                                                     \
        HT_VERIF_CODE ver_code = verify_htable(obj);                       \
        meta_info meta = {};                                                \
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
        htable_dump((obj), &(meta), "lol", LOCATION);                        \
                                                                            \
        free(meta.obj_name);                                                \
        free(meta.file_name);                                               \
        free(meta.func_name);                                               \
                                                                            \
        if(ver_code  != HT_VERIF_CODE::OK){                                 \
            return failed_red;                                              \
        }                                                                   \
    }

#elif HT_ERR_DUMP

    #define HTABLE_OK(obj, failed_red)                                          \
    {                                                                            \
        HT_VERIF_CODE ver_code = verify_htable(obj);                              \
        if(ver_code  != HT_VERIF_CODE::OK){                                        \
                meta_info meta = {};                                                \
                                                                                    \
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
                htable_dump((obj), &(meta), "lol", LOCATION);                        \
                                                                                    \
                free(meta.obj_name);                                                \
                free(meta.file_name);                                               \
                free(meta.func_name);                                               \
                                                                                    \
                return failed_red;                                                  \
        }                                                                           \
    }

#else

    #define HTABLE_OK(obj, failed_red)                \
    {                                                  \
        HT_VERIF_CODE ver_code = verify_htable(obj);    \
                                                        \
        if(ver_code  != HT_VERIF_CODE::OK){             \
            return failed_red;                          \
        }                                               \
    }

#endif //HT_TOTAL_DUMP
//========================================================================================//

//                          LOCAL_FUNCTIONS_DECLARATION

//========================================================================================//

HT_VERIF_CODE        verify_htable(const HTable* obj);
static void          htable_dump(const HTable* obj, meta_info* meta, META_PARAMS);
HT_ERR_CODE          resize(HTable* obj);

/// @brief переливание данных из dest в src
HT_ERR_CODE          transfuse_data(HTable* dest, const HashTable* src);
static void          htable_swap(HTable* obj1, HTable* obj2);

/// @brief извлекает свободную позицию в буфере хэш таблицы
static uint          extract_free_place(HTable* obj);
static void          resize_buffer(HTable* obj);
uint                 default_hash_func(const char* str);

inline uint          fill_factor_excess(double fill_factor){
    return (fill_factor < FILL_FACTOR_LIMIT) ? 0 : 1;
}

inline uint get_hash(const char* str){
    
    assert(str != NULL);
    
    uint hash = 0xFFFFFFFF;

    #if OPTIMIZE_ENABLE

        for(uint ind = 0; ind < 8; ind += 4){
            uint hash_val = *((uint*)(str + ind));
            if(hash_val == 0) break;
            hash = _mm_crc32_u32(hash, hash_val);
        }
    #else
        for(uint i = 0; str[i] != 0; i++){

            hash = (hash << 8) ^ crc32_table[((hash >> 24) ^ str[i]) & 0xFF];
        }
    #endif  // OPTIMIZE_DISABLE
    
    return hash;
}   

inline HT_VERIF_CODE verify_htable(const HTable* obj){

    assert(obj != NULL);

    if(obj->lists == NULL) return HT_VERIF_CODE::EMPTY_MEM;
    if(obj->n_lists == 0)    return HT_VERIF_CODE::SIZE_CORRUPTED;

    if(obj->hash_func == NULL)  return HT_VERIF_CODE::HASH_FUNC_CORRUPTED;

    //if(obj->n_init_lists > obj->n_reserved) return HT_VERIF_CODE::NLISTS_OVERFLOW;

#if HASH_CHECK
    for(uint n_list = 0; n_list < obj->n_lists; n_list++){

        list* cur_list = obj->lists[n_list];

        LIST_VERIF_CODE ver_code = VerifyList(cur_list);

        if(ver_code != LIST_VERIF_CODE::OK){
            return LIST_VERIF_CODE::LIST_VERIFY_FAILED;
        }

        for(uint n_elem = 0; n_elem < cur_list->size; n_elem++){
            
            #if CUSTOM_HASH_ENABLE
                uint hash_val = obj->hash_func(cur_list->nodes[n_elem].key);
            #else
                uint hash_val = get_hash(cur_list->nodes[n_elem].key);
            #endif

            if(hash_val != n_list){
                return LIST_VERIF_CODE::HASH_VALUE_INVALID;
            }
        }
    }

#elif LIST_CHECK
    for(uint n_list = 0; n_list < obj->n_lists; n_list++){

        list* cur_list = obj->lists[n_list];

        LIST_VERIF_CODE ver_code = VerifyList(cur_list);

        if(ver_code != LIST_VERIF_CODE::OK){
            return LIST_VERIF_CODE::LIST_VERIFY_FAILED;
        }
    }
#endif //HASH_CHECK

    return HT_VERIF_CODE::OK;
}

//========================================================================================//

//

//                          PUBLIC_FUNCTIONS_DEFINITION

//

//========================================================================================//

// TODO: error handling
HTable* _HTableInit(const size_t size, LOC_PARAMS){

    if(size == 0) return NULL;

    HTable* obj = (HTable*)calloc(1, sizeof(HTable));
    if(obj == NULL) return NULL;

    obj->lists = (list**)calloc(size, sizeof(list*));
    
    obj->n_lists = size;

    obj->key_buffer = (char*)aligned_alloc(ALIGN_RATIO, size * ALIGN_RATIO * sizeof(char));
    memset(obj->key_buffer, 0, size * ALIGN_RATIO * sizeof(char));
    
    obj->values = (int*)calloc(size, sizeof(int));
    obj->n_keys = 0;

    obj->pos_free.data = (int*)calloc(size, sizeof(int));
    obj->pos_free.tail = size - 1;
    obj->pos_free.head = 0;
    
    obj->n_reserved    = size;

    obj->bwords   = ListConstructor(INIT_BWORDS_SIZE);

    if(obj->lists == NULL || obj->key_buffer == NULL || obj->values == NULL || obj->pos_free.data == NULL || obj->bwords == NULL){
        
        free(obj->lists);
        free(obj->key_buffer);
        free(obj->values);
        free(obj->pos_free.data);
        free(obj->bwords);
        free(obj);

        return NULL;
    }

    obj->fill_factor = 0;
    obj->hash_func   = default_hash_func;

    for(uint n_list = 0; n_list < size; n_list++){
        obj->lists[n_list] = LST_POISON;
    }

    for(uint n_word = 0; n_word < size; n_word++){
        obj->pos_free.data[n_word] = n_word + 1;
    }

    obj->pos_free.data[size - 1] = BUSY_VAL;

    HTABLE_OK(obj, NULL)
    return obj;
}
//----------------------------------------------------------------------------------------//

HTable* _HTableInitCustomHash(const size_t size, uint (*p_func)(const char* str), LOC_PARAMS){

    if(size == 0) return NULL;

    HTable* obj = (HTable*)calloc(1, sizeof(HTable));
    if(obj == NULL) return NULL;

    obj->lists = (list**)calloc(size, sizeof(list*));
    
    obj->n_lists = size;

    obj->key_buffer = (char*)aligned_alloc(ALIGN_RATIO, size * ALIGN_RATIO * sizeof(char));
    memset(obj->key_buffer, 0, size * ALIGN_RATIO * sizeof(char));
    
    obj->values = (int*)calloc(size, sizeof(int));
    obj->n_keys = 0;

    obj->pos_free.data = (int*)calloc(size, sizeof(int));
    obj->pos_free.tail = size - 1;
    obj->pos_free.head = 0;

    obj->n_reserved = size;

    obj->bwords   = ListConstructor(INIT_BWORDS_SIZE);

    if(obj->lists == NULL || obj->key_buffer == NULL || obj->values == NULL || obj->pos_free.data == NULL || obj->bwords == NULL){
        
        free(obj->lists);
        free(obj->key_buffer);
        free(obj->values);
        free(obj->pos_free.data);
        free(obj->bwords);
        free(obj);

        return NULL;
    }

    obj->fill_factor = 0;
    obj->hash_func   = p_func;

    for(uint n_list = 0; n_list < size; n_list++){
        obj->lists[n_list] = LST_POISON;
    }

    for(uint n_word = 0; n_word < size; n_word++){
        obj->pos_free.data[n_word] = n_word + 1;
    }

    obj->pos_free.data[size - 1] = BUSY_VAL;

    HTABLE_OK(obj, NULL)
    return obj;
}
//----------------------------------------------------------------------------------------//

int _HTableFind(const HTable* obj, const char* key, int* dest, META_PARAMS){

    HTABLE_OK(obj, 0)

    assert(obj  != NULL);
    assert(key  != NULL);
    assert(dest != NULL);

    uint key_len = strlen(key);

    if(key_len >= ALIGN_RATIO){

        const list_T* p_val = ListFind(obj->bwords, key);
        if(p_val == NULL) return 0;
        
        *dest = p_val->val;
        return 1;
    }

    #if CUSTOM_HASH_ENABLE
        uint list_ind = obj->hash_func(key) % obj->n_lists;
    #else
        uint list_ind = get_hash(key) % obj->n_lists;
    #endif // CUSTOM_HASH_ENABLE

    const list* cur_list = obj->lists[list_ind];

    if(cur_list == LST_POISON){
        return 0;
    }

    #if OPTIMIZE_ENABLE
        const list_T* p_val = ListFindAligned(cur_list, key, key_len);
    #else
        const list_T* p_val = ListFind(cur_list, key);
    #endif // OPTIMIZE_ENABLE

if(p_val == NULL) return 0;
        
    *dest = p_val->val;

    HTABLE_OK(obj, 0)
    return 1;
}
//----------------------------------------------------------------------------------------//

HT_ERR_CODE     _HTableInsert(HTable* obj, const char* key, const uint value, META_PARAMS){

    assert(obj != NULL);
    assert(key != NULL);

    HTABLE_OK(obj, HT_ERR_CODE::OK)

    uint key_len = strlen(key);
    /*
    asm(
			".intel_syntax noprefix			\n\t"
			
			"mov rdi, %1                    \n\t"
            "mov rcx, 0                     \n\t"
            "mov rsi, %2                    \n\t"

            "conv_loop:                     \n\t"
            "cmp byte [rdi], 0x00                   \n\t"
            "je setting0                   \n\t"

            "mov al, byte [rdi]                 \n\t"
            "mov byte [rsi], al                 \n\t"
            "inc rdi                        \n\t"
            "inc rsi                        \n\t"
            "inc rcx                        \n\t"
            "jmp conv_loop                  \n\t"

            "after_conv:                    \n\t"
            "mov %0, ecx                    \n\t"

            "setting0:                      \n\t"
            "cmp rcx, 0x20                  \n\t"
            "je end_label                   \n\t"
            "mov byte [rsi], 0              \n\t"
            "jmp setting0                   \n\t"
            "end_label:                     \n\t"
            
			".att_syntax prefix				\n\t"

			: "=r"(key_len)
			:"r"(key), "r"(temp)
			:"rdi", "rsi", "rcx", "rax"
		);
    
    printf("%s %s %u\n", key, temp, key_len);
    exit(0);
    */


    if(key_len >= ALIGN_RATIO){
        list_T* p_val = ListFind(obj->bwords, key);
        if(p_val == NULL){
            uint free_pos   = extract_free_place(obj);

            list_T new_elem = {
                obj->key_buffer + free_pos,
                (uint)value
            };
            PushBack(obj->bwords, new_elem);
        }
        else{
            // update value
            p_val->val = value;
        }
        HTABLE_OK(obj, HT_ERR_CODE::OK)
        return HT_ERR_CODE::OK;
    }

    #if CUSTOM_HASH_ENABLE    
        uint list_ind = obj->hash_func(key) % obj->n_lists;
    #else
        uint list_ind = get_hash(key) % obj->n_lists;
    #endif // CUSTOM_HASH_ENABLE
        
    if(obj->lists[list_ind] == LST_POISON){
        obj->lists[list_ind] = ListConstructor(1);
        obj->n_init_lists++;
    }
    else{
        #if OPTIMIZE_ENABLE
            list_T* p_val = ListFindAligned(obj->lists[list_ind], key, key_len);
        #else
            list_T* p_val = ListFind(obj->lists[list_ind], key);
        #endif // OPTIMIZE_ENABLE

        if(p_val != NULL){
            p_val->val = value;

            HTABLE_OK(obj, HT_ERR_CODE::OK)
            return HT_ERR_CODE::OK;
        }
    }

    uint free_pos   = extract_free_place(obj);

    list_T new_elem = {
        obj->key_buffer + free_pos,
        (uint)value
    };

    memset(obj->key_buffer + free_pos, 0, 32);
    strcpy(obj->key_buffer + free_pos, key);
    PushBack(obj->lists[list_ind], new_elem);

    obj->n_keys++;

    #if RESIZE_ENABLE
        obj->fill_factor = (double)obj->n_keys / (double)obj->n_lists;
        
        if(fill_factor_excess(obj->fill_factor)){
            resize(obj);
        }
    #endif //   RESIZE_ENABLE
    
    HTABLE_OK(obj, HT_ERR_CODE::OK)
    return HT_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

void _HTableRemove(HTable* obj, META_PARAMS){

    if(obj == NULL) return;

    for(uint n_list = 0; n_list < obj->n_lists; n_list++){
        if(obj->lists[n_list] != LST_POISON){
            ListDestructor(obj->lists[n_list]);
        }
    }
    ListDestructor(obj->bwords);
    
    free(obj->lists);
    free(obj->key_buffer);
    free(obj->values);
    free(obj);

    return;
}
//----------------------------------------------------------------------------------------//

//========================================================================================//

//

//                          LOCAL_FUNCTIONS_DEFINITION

//

//========================================================================================//


static void htable_dump(const HTable* obj, meta_info* meta, META_PARAMS){

    assert(obj  != NULL);
    assert(meta != NULL);

    LOG("<br><br>Dump called from function %s(%d)<br>"
        "That was called from file %s, function %s(%d)<br>"
        "[#] hash table %s<int><br>"
        "<br>"
        "lists              = %p<br>"
        "n_lists            = %lu<br>"
        "key_buffer         = %p<br>"
        "values             = %p<br>"
        "n_keys             = %lu<br>"
        "pos_free.data      = %p<br>"
        "pos_free.head      = %p<br>"
        "pos_free.tail      = %p<br>"
        "bwords             = %p<br>"
        "fill_factor        = %g<br>"
        "n_init_lists       = %lu<br>"
        "<br>",
        func_name, n_line, meta->file_name, meta->func_name, meta->n_line,
        meta->obj_name,
        obj->lists, obj->n_lists,
        obj->key_buffer, obj->values, obj->n_keys,
        obj->pos_free.data, obj->pos_free.head, obj->pos_free.tail,
        obj->bwords, obj->fill_factor, obj->n_init_lists);

    LOG("<br>");

    for(uint n_list = 0; n_list < obj->n_lists; n_list++){

        if(obj->lists[n_list] != LST_POISON){
            LOG("<br>List: %u: <br>", n_list);
            #if LIST_FULL_INFO_DUMP
                ListDump(obj->lists[n_list], meta, META_PARAMS);
            #else
                DumpNodes(obj->lists[n_list]);
            #endif // LIST_FULL_INFO_DUMP
        }
    }

    LOG("<br><br>=====================================================================================<br><br>");

    return;
}
//----------------------------------------------------------------------------------------//

HT_ERR_CODE resize(HTable* obj){

    assert(obj  != NULL);

    size_t new_nlists = obj->n_lists << INCREASE_RATIO;
    
    #if CUSTOM_HASH_ENABLE
        HTable* new_obj = HTableInitCustomHash(new_nlists, obj->hash_func);
    #else
        HTable* new_obj = HTableInit(new_nlists);
    #endif // CUSTOM_HASH_ENABLE

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

    uint total_n_key = 0;

    for(uint n_list = 0; n_list < src->n_lists; n_list++){

        list* cur_list = src->lists[n_list];

        if(cur_list != LST_POISON){
            for(uint n_key = 0; n_key < cur_list->size; n_key++){
                
                list_T cur_elem = cur_list->nodes[n_key].val;

                list_T new_elem = {
                    dest->key_buffer + total_n_key * ALIGN_RATIO,
                    cur_elem.val
                };

                #if CUSTOM_HASH_ENABLE    
                    uint list_ind = dest->hash_func(cur_elem.p_key) % dest->n_lists;
                #else
                    uint list_ind = get_hash(cur_elem.p_key) % dest->n_lists;
                #endif // CUSTOM_HASH_ENABLE

                if(dest->lists[list_ind] == LST_POISON){
                    dest->lists[list_ind] = ListConstructor(1);
                    dest->n_init_lists++;
                }

                strcpy(dest->key_buffer + total_n_key * ALIGN_RATIO, cur_elem.p_key);
                PushBack(dest->lists[list_ind], new_elem);

                total_n_key++;
            }
        }
    }
    dest->n_keys = total_n_key;
    dest->fill_factor = (double)dest->n_keys / (double)dest->n_lists;

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

static uint extract_free_place(HTable* obj){

    assert(obj != NULL);

    int n_elem = obj->pos_free.head;

    if(n_elem == BUSY_VAL){
        resize_buffer(obj);
        n_elem = obj->pos_free.head;
    }

    int next_elem_ind             = obj->pos_free.data[n_elem];
    obj->pos_free.data[n_elem]    = BUSY_VAL;
    obj->pos_free.head            = next_elem_ind;

    return n_elem * ALIGN_RATIO;
}
//----------------------------------------------------------------------------------------//

static void resize_buffer(HTable* obj){

    assert(obj != NULL);

    size_t new_size = ALIGN_RATIO * obj->n_reserved * BUF_INCREASE_RATIO;
    
    char* new_buffer = (char*)aligned_alloc(ALIGN_RATIO, obj->n_reserved * BUF_INCREASE_RATIO * ALIGN_RATIO * sizeof(char));
    assert(new_buffer != NULL);
    memset(new_buffer, 0, obj->n_reserved * BUF_INCREASE_RATIO * ALIGN_RATIO * sizeof(char));
    
    for(uint n_list = 0; n_list < obj->n_lists; n_list++){

        list* cur_list = obj->lists[n_list];
        if(cur_list == LST_POISON) continue;

        size_t list_size = cur_list->size;

        for(uint n_word = 0; n_word < list_size; n_word++){
            
            cur_list->nodes[n_word].val.p_key = new_buffer + (uint)((char*)cur_list->nodes[n_word].val.p_key - obj->key_buffer);
        }
    }

    memcpy(new_buffer, obj->key_buffer, obj->n_reserved * ALIGN_RATIO);

    int* new_pos_free_data = (int*)calloc(obj->n_reserved * BUF_INCREASE_RATIO, sizeof(int));
    assert(new_pos_free_data != NULL);

    memcpy(new_pos_free_data, obj->pos_free.data, obj->n_reserved);

    for(uint i = obj->n_reserved; i < obj->n_reserved * BUF_INCREASE_RATIO; i++){
        new_pos_free_data[i] = i + 1;
    }
    new_pos_free_data[obj->n_reserved * BUF_INCREASE_RATIO - 1] = BUSY_VAL;

    if(obj->pos_free.head == BUSY_VAL){
        obj->pos_free.head = obj->n_reserved;
    }
    else{
        obj->pos_free.data[obj->pos_free.tail] = obj->n_reserved;
    }
    obj->pos_free.tail = obj->n_reserved * BUF_INCREASE_RATIO - 1;

    free(obj->key_buffer);
    free(obj->pos_free.data);

    obj->key_buffer = new_buffer;
    obj->pos_free.data = new_pos_free_data;

    obj->n_reserved *= BUF_INCREASE_RATIO;

    return;
}
//----------------------------------------------------------------------------------------//

uint default_hash_func(const char* str){
    
    assert(str != NULL);

    uint hash = 0xFFFFFFFF;

    #if OPTIMIZE_ENABLE
    printf("123\n");
        for(uint ind = 0; ind < 32; ind += 8){
            uint hash_val = *((unsigned long long*)(str + ind));
            if(hash_val == 0) break;
            hash = _mm_crc32_u64(hash, hash_val);
        }
    #else
        for(uint i = 0; str[i] != 0; i++){

            hash = (hash << 8) ^ crc32_table[((hash >> 24) ^ str[i]) & 0xFF];
        }
            
    #endif  // OPTIMIZE_DISABLE
    
    return hash;
}   
//----------------------------------------------------------------------------------------//
