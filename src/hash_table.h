#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "list.h"

// TODO: указатель на хэш функцию - поле в структуре, хм
struct HashTable{
    list** data;
    uint (*hash_func)(const char* str);
    size_t size;
}; 

enum HT_ERR_CODE{

    OK,
    ALLOC_MEM,
};

typedef HashTable Htabl;

Htabl*  HTableInit(const size_t size, uint (*hash_func)(const char* str));
uint    HTableFind(Htabl* obj, list_T str);
void    HTableInsert(Htabl* obj, list_T str);
void    HTableRemove(Htabl* obj);
uint    IsListEmpty(const Htabl* obj, const uint n_list);

#endif //HASH_TABLE_H
