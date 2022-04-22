# hash_table
реализация хэш таблицы с закрытой адресацией

## Оптимизационная часть
Оценку производительности хэш таблицы будем проводить с помощью профайлера callgrind. Составим сценарий, при котором у нас будет вставка уникальных слов из текста в таблицу, а после - поиск каждого слова исходного текста в таблице. То есть нагрузочная функция будет иметь следующий вид:
```cpp

void UseHtable(const text_storage* text, const text_storage* unique_text, const size_t hash_size){
    
    Htabl* htable = HTableInit(hash_size, HashCRC32);
    assert(htable != NULL);

    for(uint n_elem = 0; n_elem < unique_text->n_words; n_elem++){
        HTableInsert(htable, unique_text->p_words[n_elem].pt);
    }

    for(uint n_word = 0; n_word < text->n_words; n_word++){
        HTableFind(htable, text->p_words[n_word].pt);
    }

    return;
}

```

Запустим программу на тестовых данных и посмотрим, какую информацию нам выдаст профайлер. Посмотрим нагрузку каждой функции в процентном соотношении от функции UseHTable:
![изображение](https://user-images.githubusercontent.com/89589647/164607898-6ec4ef13-7ab9-40e1-ae57-782bb3ca49ed.png)

Нетрудно заметить, что самыми прожорливыми являются функции get_hash, HTableFind, ListFind, __strcmp_avx2, verify htable.

### оптимизация 1
Начнём с функции get_hash. В ней по строке вычисляется хэш по алгоритму crc32.

```cpp
static uint get_hash(const char* str){
    
    assert(str != NULL);

    uint hash = 0xFFFFFFFF;

    for(uint i = 0; str[i] != 0; i++){

        hash = (hash << 8) ^ crc32_table[((hash >> 24) ^ str[i]) & 0xFF];
    }

    return hash;
}
```
# Спектральный анализ

Проведём спектральный анализ нашей хэш таблицы на 6 хэш функциях для того, чтобы оценить их эффективность. Для этого зафиксируем размер таблицы.

![изображение](https://user-images.githubusercontent.com/89589647/164605821-4d80e589-b3a3-4115-9ed4-065b33f3c916.png)
Нетрудно заметить, что функции возврата константы, ascii первого символа строки и длины строки образуют один больший кластер, вследствие чего хэш функции являются чудовищно плохими. Контрольная сумма уже получше, хотя всё также образует один большой кластер вначале структуры. Rol hash еще лучше, значение дисперсии - 13.8. А победителем само собой является crc32 с дисперсией 4.89.
