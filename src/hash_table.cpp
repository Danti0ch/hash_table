#include "hash_table.h"
#include <stdlib.h>

static list* LST_POISON = (list*)123;

// TODO: сделать рефактор
//  dump
//  verification
//  erase
//  come up with new stuff???

//                  I PART                                      
// TODO: запустить на входных данных
// TODO: запилить функцию сохранения баров в html
// TODO: readme?                                            
// TODO:  запилить crc32
// TODO: интерфейс
//                  II PART
// TODO: запустить профайлер                            
// TODO: оптимизировать функции, которые больше всех жрут

// TODO: желательно поменториться у кого-нибудь
// TODO: (опционально) улучшить изображение гистограмм
// TODO: усилить модуль логирования

//----------------------PUBLIC-FUNCTIONS-DEFINITIONS----------------------//

Htabl* HTableInit(const size_t size, uint (*hash_func)(const char* str)){

    assert(hash_func != NULL);

    if(size == 0) return NULL;

    Htabl* obj = (Htabl*)calloc(1, sizeof(Htabl));
    if(obj == NULL) return NULL;

    obj->data = (list**)calloc(size, sizeof(list*));
    if(obj->data == NULL){
        free(obj);
        //return ALLOC_MEM;
        return NULL;
    }

    obj->size      = size;
    obj->hash_func = hash_func;

    for(uint n_list = 0; n_list < size; n_list++){
        obj->data[n_list] = LST_POISON;
    }
    return obj;
}
//----------------------------------------------------------------------------------------//

// TODO: fox char* -> cosnt char*
uint HTableFind(Htabl* obj, list_T str){

    assert(obj != NULL);
    assert(str != NULL);

    uint list_ind = obj->hash_func(str) % obj->size;

    if(list_ind >= obj->size || obj->data[list_ind] == LST_POISON) return 0;

    node* res = ListFind(obj->data[list_ind], str);

    if(res == NULL) return 0;

    return 1;
}
//----------------------------------------------------------------------------------------//

void HTableInsert(Htabl* obj, list_T str){

    assert(obj != NULL);

    uint list_ind = obj->hash_func(str) % obj->size;

    assert(list_ind < obj->size);

    if(obj->data[list_ind] == LST_POISON){
        obj->data[list_ind] = ListConstructor(1);
    }

    if(ListFind(obj->data[list_ind], str) == NULL){
        PushBack(obj->data[list_ind], str);
    }

    return;
}
//----------------------------------------------------------------------------------------//

void HTableRemove(Htabl* obj){

    if(obj == NULL) return;

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

uint IsListEmpty(const Htabl* obj, const uint n_list){

    assert(obj != NULL);

    if(obj->data[n_list] == LST_POISON) return 1;

    return 0;
}
//----------------------------------------------------------------------------------------//
