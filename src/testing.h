#ifndef TESTING_H
#define TESTING_H

#include "hash_table.h"
#include "text_storage.h"
const char TEMP_FILE_NAME[] = "temp";
const uint MAX_HASH_SIZE    = 1 << 30 - 1;

enum BAR_DRAW_MODE{
    multiple_images,
    one_image
};

/**
 * @brief функция строит гистограммы распределения длин вторичных структур для хэш таблицы по хэш функциям, которые указаны в фалй hash_funcs
 * 
 * @param data указатель на массив данных, которыми будет заполняться хэш таблица
 * @param htable_size размер хэш таблицы
 * @param image_name название картинки, в которую будет происходить графический вывод результата спектрального анализа
 */
void GetSpectralAnalysis(const list_T* data, const size_t n_elems, const size_t htable_size, const char* image_name, const BAR_DRAW_MODE save_mode, const uint is_show);
void UseHtable(const text_storage* text, const text_storage* unique_text, const size_t hash_size);

#endif // TESTING_H
