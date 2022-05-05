#ifndef TESTING_H
#define TESTING_H

#include "structs/hash_table.h"
#include "structs/text_storage.h"

const char TEMP_FILE_NAME[] = "temp";
const uint MAX_HASH_SIZE    = 1 << 30 - 1;

enum BAR_DRAW_MODE{
    MULTIPLE_IMAGES,
    ONE_IMAGE
};

enum USAGE_MODE{
    LOAD,
    SPECTRAL_ANALYSIS
};

const char DEFAULT_DICT_FILE_NAME[]           = "words.txt";
const uint DEFAULT_HT_SIZE                  = 1019;
const char DEFAULT_FREQ_STAT_FILE_NAME[]    = "enwik8";
const uint DEFAULT_TESTS_NUMBER             = 1;

/**
 * @brief функция строит гистограммы распределения длин вторичных структур для хэш таблицы по хэш функциям, которые указаны в фалй hash_funcs
 * 
 * @param data указатель на массив данных, которыми будет заполняться хэш таблица
 * @param htable_size размер хэш таблицы
 * @param image_name название картинки, в которую будет происходить графический вывод результата спектрального анализа
 */
void GetSpectralAnalysis(const text_storage* data, const size_t htable_size, const BAR_DRAW_MODE save_mode, const uint is_show);
void LoadHTable(const text_storage* text, const text_storage* dict, const size_t htable_size);
void InitTesting();

#endif // TESTING_H
