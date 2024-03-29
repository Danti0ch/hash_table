#include "hash_table.h"
#include <stdlib.h>
#include "logger.h"
#include <assert.h>
#include <nmmintrin.h>
#include <math.h>
#include <string.h>

static list* LST_POISON = (list*)123;
// TODO: readme

const int INCREASE_RATIO = 1;

// режим для resize
const int INCREASE_MODE  = 1;
const int REDUCE_MODE    = 0;

static const uint crc32_table[] =
{
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

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

static HT_VERIF_CODE verify_htable(const HTable* obj);
static void          htable_dump(const HTable* obj, meta_info* meta, META_PARAMS);
static uint          get_hash(const char* str);
static uint          fill_factor_excess(double fill_factor);
HT_ERR_CODE          resize(HTable* obj);
HT_ERR_CODE          transfuse_data(HTable* dest, const HashTable* src);
static void          htable_swap(HTable* obj1, HTable* obj2);
static uint          extract_free_place(HTable* obj);
static void          resize_buffer(HTable* obj);

//========================================================================================//

//                          PUBLIC_FUNCTIONS_DEFINITION

//========================================================================================//

HT_ERR_CODE _HTableInit(HTable** obj, const size_t size, LOC_PARAMS){

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
    
    uint list_ind = obj->hash_func(str) % obj->size;

    if(obj->data[list_ind] == LST_POISON){
        *is_in_table = 0;
        return HT_ERR_CODE::OK;
    }

    node* res = ListFind(obj->data[list_ind], str);

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

uint IsListEmpty(const HTable* obj, const uint n_list){

    assert(obj != NULL);

    if(obj->data[n_list] == LST_POISON) return 1;

    return 0;
}
//----------------------------------------------------------------------------------------//

HT_ERR_CODE _HTableGetSize(HTable* obj, size_t* dest, META_PARAMS){

    assert(obj  != NULL);
    assert(dest != NULL);
    HTABLE_OK(obj)

    *dest = obj->size;

    return HT_ERR_CODE::OK;
}
//========================================================================================//

//

//                          LOCAL_FUNCTIONS_DEFINITION

//

//========================================================================================//

static HT_VERIF_CODE verify_htable(const HTable* obj){

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

static uint get_hash(const char* str){
    
    assert(str != NULL);

    #if OPTIMIZE_DISABLE
        uint hash = 0xFFFFFFFF;

        for(uint i = 0; str[i] != 0; i++){

            hash = (hash << 8) ^ crc32_table[((hash >> 24) ^ str[i]) & 0xFF];
        }
    #else
        uint hash = _mm_crc32_u32(0xFFFFFFFF, *((uint*)(str)));
    #endif  // OPTIMIZE_DISABLE

    return hash;
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
