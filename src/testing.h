#ifndef TESTING_H
#define TESTING_H

#include "hash_table.h"

const char TEMP_FILE_NAME[] = "temp";
const uint MAX_HASH_SIZE    = 1 << 30 - 1;

/**
 * @brief функция строит гистограммы распределения длин вторичных структур для хэш таблицы по хэш функциям, которые указаны в фалй hash_funcs
 * 
 * @param data указатель на массив данных, которыми будет заполняться хэш таблица
 * @param htable_size размер хэш таблицы
 * @param image_name название картинки, в которую будет происходить графический вывод результата спектрального анализа
 */
void GetSpectralAnalysis(const list_T* data, const size_t n_elems, const size_t htable_size, const char* image_name);

#endif // TESTING_H
