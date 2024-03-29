#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "list.h"

// TODO: указатель на хэш функцию - поле в структуре, хм
typedef struct HashTable{
    list** data;
    size_t size;
    size_t n_elems;

    char* buffer;
    size_t n_words;
    
    uint* buf_free_vals;
    uint free_tail;
    uint free_head;

    list* bwords;

    double fill_factor;
    uint (*hash_func)(const char* str);
    // количество памяти выделенное под каждый список
    size_t total_mem_size;  // ??
    size_t n_init_lists;
} HTable;

const double FILL_FACTOR_LIMIT = 0.6;

#ifndef CUSTOM_HASH_ENABLE
    #define CUSTOM_HASH_ENABLE 1
#endif

#ifndef OPTIMIZE_DISABLE
    #define OPTIMIZE_DISABLE 0
#endif

#ifndef RESIZE_ENABLE
    #define RESIZE_ENABLE 1
#endif

#ifndef LIST_CHECK
    #define LIST_CHECK 0
#endif

// проверка хэша каждого элемента
#ifndef HASH_CHECK
    #define HASH_CHECK 0
#endif

#ifndef HT_ERR_DUMP
    #define HT_ERR_DUMP 0
#endif

#ifndef HT_TOTAL_DUMP
    #define HT_TOTAL_DUMP 0
#endif

#ifndef LIST_FULL_INFO_DUMP
    #define LIST_FULL_INFO_DUMP 0
#endif

enum class HT_VERIF_CODE{
    OK,
    EMPTY_MEM,
    SIZE_CORRUPTED,
    NLISTS_OVERFLOW,
    LIST_VERIFY_FAILED,
    HASH_VALUE_INVALID,
    HASH_FUNC_CORRUPTED
};

enum class HT_ERR_CODE{

    OK,
    ALLOC_MEM,
    INVALID_FREE_TABLE,
    INVALID_SIZE,
    UNABLE_ALLOC_MEM,
    VERIFY_FAILED
};

#ifndef LOCATION
    #define LOCATION __FILE__, __FUNCTION__, __LINE__
#endif
#ifndef META_PARAMS
    #define META_PARAMS char const * obj_name, char const * file_name, char const * func_name, int const n_line
#endif
#ifndef LOC_PARAMS
    #define LOC_PARAMS  char const * file_name, char const * func_name, int const n_line
#endif

#define HTableInit(obj, size)           _HTableInit((obj), (size), LOCATION)
#define HTableFind(obj, elem, res)      _HTableFind((obj), (elem), (res), #obj, LOCATION)
#define HTableInsert(obj, elem)         _HTableInsert((obj), (elem), #obj, LOCATION)
#define HTableRemove(obj)               _HTableRemove((obj), #obj, LOCATION)
#define GetSize(obj, dest)              _HTableGetSize(obj, (dest), #obj, LOCATION)

#if CUSTOM_HASH_ENABLE
    #define HTableInitCustomHash(obj, size, func)     _HTableInitCustomHash((obj), (size), (func), LOCATION)
#endif
HT_ERR_CODE     _HTableInit(HTable** obj, const size_t size, LOC_PARAMS);
#if CUSTOM_HASH_ENABLE
    HT_ERR_CODE     _HTableInitCustomHash(HTable** obj, const size_t size, uint (*p_func)(const char* str), LOC_PARAMS);
#endif

HT_ERR_CODE     _HTableFind(const HTable* obj, const list_T str, uint* is_in_table, META_PARAMS);
HT_ERR_CODE     _HTableInsert(HTable* obj, const list_T str, META_PARAMS);
void            _HTableRemove(HTable* obj, META_PARAMS);
HT_ERR_CODE     _HTableGetSize(HTable* obj, size_t* dest, META_PARAMS);

uint IsListEmpty(const HTable* obj, const uint n_list);

#endif //HASH_TABLE_H
