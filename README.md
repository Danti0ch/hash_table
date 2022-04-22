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
# Спектральный анализ

Проведём спектральный анализ нашей хэш таблицы на 6 хэш функциях для того, чтобы оценить их эффективность. Для этого зафиксируем размер таблицы.

![изображение](https://user-images.githubusercontent.com/89589647/164605821-4d80e589-b3a3-4115-9ed4-065b33f3c916.png)
Нетрудно заметить, что функции возврата константы, ascii первого символа строки и длины строки образуют один больший кластер, вследствие чего хэш функции являются чудовищно плохими. Контрольная сумма уже получше, хотя всё также образует один большой кластер вначале структуры. Rol hash еще лучше, значение дисперсии - 13.8. А победителем само собой является crc32 с дисперсией 4.89.
