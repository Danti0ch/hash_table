# hash_table
реализация хэш таблицы с закрытой адресацией

## Оптимизационная часть

### Оценка ускорения
Мы будем использовать профайлер callgrind, который показывает в процентах долю затрат каждой функции. Будем оценивать производительность нагрузочной функции с помощью функции rdtsc().

Также мы имеем метрику оценки производительности с учётом количества ассемблерных вставок: 

```
N/M * 1000
```
Где N - общее ускорение программы, а M - количество ассемблерых строк.

### Оптимизации
Будем использовать таблицу так: предположим, что у нас есть очень большой файл A, который полностью поместиться в таблицу не сможет. Возьмем файл-словарь B, загрузим из него слова в таблицу и будем считать частоту появления слов из словаря B в большом файле A.

В качестве словаря B возьмем файл words.txt, а в качестве файла A - файл enwik8.

Начальный размер хэш таблицы - 1019

Оценку производительности хэш таблицы будем проводить с помощью профайлера callgrind и здравого смысла.

Нагрузочная функция будет иметь следующий вид:
```cpp

void LoadHTable(const text_storage* text, const text_storage* dict, const size_t htable_size){
    
    assert(text != NULL);
    assert(dict != NULL);

    HTable* htable = HTableInit(htable_size);
    assert(htable != NULL);

    for(uint n_word = 0; n_word < dict->n_words; n_word++){
        if(n_word % 100000  == 0) printf("pack %u loaded\n", n_word);
        HTableInsert(htable, GetWord(dict, n_word), 0);
    }

    for(uint n_word = 0; n_word < text->n_words; n_word++){

        int cur_freq = 0;
        const char* cur_word = GetWord(text, n_word);

        int isfind = HTableFind(htable, cur_word, &cur_freq);
        if(isfind){
            HTableInsert(htable, cur_word, cur_freq + 1);
        }

        if(n_word % 1000000  == 0){
            printf("%u pack found\n", n_word);
        }
    }

    HTableRemove(htable);
    return;
}
```

Запустим программу на тестовых данных и посмотрим, какую информацию нам выдаст профайлер. Посмотрим нагрузку каждой функции в процентном соотношении от функции LoadHTable:
![изображение](https://user-images.githubusercontent.com/89589647/167061970-cf92d01c-f1e3-4fc2-a8d9-d9ef83078ddf.png)

Также был сделан замер по результатам которого на выполнение функции LoadHTable ушло 2.13 секунд(усредненое значение с 32 тестов).

### оптимизация 1

Нетрудно заметить, что функция HTableFind тратит больше всего ресурсов. Посмотрим какой конкретно участок кода потребляет больше всего ресурсов

![изображение](https://user-images.githubusercontent.com/89589647/167070236-e0bc85fe-18ed-4496-b72a-40530bc68a25.png)
Оказывается это хэш функция. Линейное вычисление хэша в совокупности с частым обращением к памяти приводит к тому, что хэш функция тратит много ресурсов. Воспользуемся тем, что строки в таблице ограничены длиной 32 и будем вычислять хэш функцию через simd инструкцию вычисления crc32. Будем считать сразу по 32 бита. Также сделаем эту функцию inline

```cpp
inline uint get_hash(const char* str){
    
    assert(str != NULL);
    
    uint hash = 0xFFFFFFFF;

    #if OPTIMIZE_ENABLE

        for(uint ind = 0; ind < 8; ind += 4){
            uint hash_val = *((uint*)(str + ind));
            if(hash_val == 0) break;
            hash = _mm_crc32_u32(hash, hash_val);
        }
    #else
        for(uint i = 0; str[i] != 0; i++){

            hash = (hash << 8) ^ crc32_table[((hash >> 24) ^ str[i]) & 0xFF];
        }
    #endif  // OPTIMIZE_DISABLE
    
    return hash;
}

```

Теперь на выполнение LoadHTable уходит 1.63 секунд. Ускорение на 30%.

### оптимизация 2

Посмотрим теперь на вывод профайлера:
![изображение](https://user-images.githubusercontent.com/89589647/167071884-a9358767-284b-4c34-ba34-145ab6cbe905.png)

Перейдем к выводу all calees для самой затратной функции HTableFind

![изображение](https://user-images.githubusercontent.com/89589647/167072009-84fa1a27-f82c-4f5e-ba7e-36fd01da1406.png)

Перейдем к выводу all calees для самой затратной функции ListFind
![изображение](https://user-images.githubusercontent.com/89589647/167072066-ac155b2e-9876-4ba4-92d6-b1b14c4f0b06.png)

На strcmp уходит около половины ресурсов при выполнении функции ListFind. Попробуем её прооптимизировать. Опять же воспользуемся тем, что строки в таблице содержатся в ячейках по 32 байта. Воспользуемся inline asm чтобы через ymm регистры выполнять сравнение за O(1), а не за O(n). Напишем для этого отдельную функцию ListFindAligned.

```cpp
list_T* _ListFindAligned(const list* obj, const char* key, const size_t key_len, META_PARAMS){
	
	LIST_OK(obj)

	node* cur_node = obj->nodes + obj->head;

	memcpy(temp, key, key_len);
	memset(temp + key_len, 0, ALIGN_RATIO - key_len);

	for(uint n_node = 0; n_node < obj->size; n_node++){

		uint res = 0;
		
		asm(
			".intel_syntax noprefix			\n\t"
			
			"vmovdqa ymm0, [%1]				\n\t"
			"vmovdqu ymm1, [%2]				\n\t"
			"vpcmpeqb ymm2, ymm0, ymm1		\n\t"

			"vpmovmskb eax, ymm2			\n\t"
			"mov %0, eax					\n\t"

			".att_syntax prefix				\n\t"

			:"=r"(res)
			:"r"(cur_node[n_node].val.p_key), "r"(temp)
			:"ymm0", "ymm1", "ymm2", "rax"
		);
		
		if(res == 0xFFFFFFFF) return &(cur_node[n_node].val);

		#if ENABLE_SORT == 0
			cur_node = obj->nodes + cur_node->next;
		#endif
	}

	return NULL;
}
```
Генеральная функция выполняется уже за 1.57 секунд
Ускорение программы выросло еще на 6%

### оптимизация 3
![изображение](https://user-images.githubusercontent.com/89589647/167083112-d259a75e-4668-40af-9339-fe3f96c129e7.png)

 ListFindAligned является очень весомой функцией.
 
 ![изображение](https://user-images.githubusercontent.com/89589647/167083197-5f541da4-471a-46d5-bf08-b28dd03854e4.png)

При вызове HTableFind и HTableInsert почти всегда происходит вызов ListFindALigned и всегда в начале функций происходит расчёт длины ключа через функцию strlen.

![изображение](https://user-images.githubusercontent.com/89589647/167083284-606d9da0-66df-4210-bdda-63095e9822fe.png)

Почему бы нам не обьеденить несколько строковых функций в одну, чтобы уменьшить затраты на вызовы функции и грамотней воспользоваться регистрами? Напишем функцию lencpset которая будет возвращать длину ключа и устанавливать строку temp, в которую происходит копирование ключа, нулями(то есть это strlen + memset).

```nasm
section .text

global lencpset

;===============================================================
; args:
;   1) dest   - pointer to dest string to copy key and memset 0 
;   2) src    - pointer to key string that we need to copy and get len
; ret: 
;   rax - src len
;==============================================================
lencpset:
    xor rax, rax

    vpxor ymm0, ymm0
    vpcmpeqb ymm1, ymm0, [rsi]
    vpmovmskb edx, ymm1

    bsf eax, edx

    vmovdqu [rdi], ymm0

    ret

```

LoadHTable выполняется теперь за 1.46 секунды. Производительность выросла еще на 10%
### итог
Программа была ускорена на 46%.
Было написано 13 строчек на ассемблере, тем самым наша главная метрика равна

```
k = 1.46/13 * 1000 = 112
```

# Спектральный анализ

Проведём спектральный анализ нашей хэш таблицы на 6 хэш функциях для того, чтобы оценить их эффективность. Для этого зафиксируем размер таблицы.
![изображение](https://user-images.githubusercontent.com/89589647/167064045-517d1c23-2a21-43c1-848c-c4252159a061.png)

Нетрудно заметить, что функции возврата константы, ascii первого символа строки и длины строки образуют один больший кластер, вследствие чего хэш функции являются чудовищно плохими. Контрольная сумма уже получше, хотя всё также образует один большой кластер вначале структуры. Rol hash еще лучше, значение дисперсии - 50.14. А победителем само собой является crc32 с дисперсией 6.1.
