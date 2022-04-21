#include "hash_table.h"
#include <stdlib.h>
#include "logger.h"
#include <assert.h>
#include <nmmintrin.h>

static list* LST_POISON = (list*)123;

//                  I PART                       
// TODO: readme?                                            

//                  II PART
// TODO: запустить профайлер                            
// TODO: оптимизировать функции, которые больше всех жрут

// TODO: желательно поменториться у кого-нибудь
// TODO: (опционально) улучшить изображение гистограмм
// TODO: усилить модуль логирования


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
static void          htable_dump(const HTable* obj);
static uint          get_hash(const char* str);

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

    (*obj)->size           = size;
    (*obj)->hash_func      = get_hash;

    for(uint n_list = 0; n_list < size; n_list++){
        (*obj)->data[n_list] = LST_POISON;
    }

    HTABLE_OK(*obj)
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

    (*obj)->size           = size;
    (*obj)->hash_func      = p_func;

    for(uint n_list = 0; n_list < size; n_list++){
        (*obj)->data[n_list] = LST_POISON;
    }

    HTABLE_OK(*obj)
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
        obj->data[list_ind] = ListConstructor(1);
    }

    if(ListFind(obj->data[list_ind], str) == NULL){
        PushBack(obj->data[list_ind], str);
    }

    HTABLE_OK(obj)
    return HT_ERR_CODE::OK;
}
//----------------------------------------------------------------------------------------//

void _HTableRemove(HTable* obj, META_PARAMS){

    if(obj == NULL) return;
    HTABLE_OK(obj)

    for(uint n_list = 0; n_list < obj->size; n_list++){
        if(obj->data[n_list] != LST_POISON){
            ListDestructor(obj->data[n_list]);
        }
    }

    free(obj->data);
    free(obj);

    return;
}
//----------------------------------------------------------------------------------------//

uint IsListEmpty(const HTable* obj, const uint n_list){

    assert(obj != NULL);
    HTABLE_OK(obj)

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
        LOG("<br>List: %d: <br>");
        if(obj->data[n_list] != LST_POISON){

            #if LIST_FULL_INFO_DUMP
                ListDump(obj->data[n_list], meta, META_PARAMS);
            #else
                DumpNodes(obj->data[n_list]);
            #endif // LIST_FULL_INFO_DUMP
        }
    }

    LOG("<br><br>");
}
//----------------------------------------------------------------------------------------//

static uint get_hash(const char* str){
    
    assert(str != NULL);

    uint hash = 0xFFFFFFFF;

    for(uint i = 0; str[i] != 0; i++){

        hash = _mm_crc32_u8(hash, str[i]);
    }

    return hash;
}
//----------------------------------------------------------------------------------------//
