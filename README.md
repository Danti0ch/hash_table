# hash_table
реализация хэш таблицы с закрытой адресацией

## Оптимизационная часть

### Оценка ускорения
Мы будем использовать профайлер callgrind, который показывает в процентах долю затрат каждой функции. Будем оценивать производительность нагрузочной функции через замер времени(clock_t).

Также мы имеем метрику оценки производительности с учётом количества ассемблерных вставок: 
```
N/M * 1000
```
Где N - общее ускорение программы, а M - количество ассемблерых строк.

### Оптимизации
В качестве словаря возьму файл words.txt, а в качестве текста - enwik8. Начальный размер хэш таблицы на всех тестах будет 1019.


Оценку производительности хэш таблицы будем проводить с помощью профайлера callgrind. Составим сценарий, при котором у нас происходит замер на частоту нахождения слов из словаря в тексте. То есть вначале будет вставка уникальных слов из словаря в таблицу, а после - поиск слов из текста. 
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
file:///home/plato/mipt_projs/hash_table/readme_images/no_opts.png![изображение](https://user-images.githubusercontent.com/89589647/167061970-cf92d01c-f1e3-4fc2-a8d9-d9ef83078ddf.png)

Также был сделан замер по результатам которого на выполнение функции LoadTable ушло 2.13 секунд(усредненое значение с 32 тестов).

### оптимизация 1

Нетрудно заметить, что функция HTableFind тратит больше всего ресурсов. Посмотрим какой конкретно участок кода потребляет больше всего ресурсов

file:///home/plato/mipt_projs/hash_table/readme_images/ht_find_no_opts_listing.png![изображение](https://user-images.githubusercontent.com/89589647/167070236-e0bc85fe-18ed-4496-b72a-40530bc68a25.png)
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
file:///home/plato/mipt_projs/hash_table/readme_images/opt1_1.png![изображение](https://user-images.githubusercontent.com/89589647/167071884-a9358767-284b-4c34-ba34-145ab6cbe905.png)

Перейдем к выводу all calees для самой затратной функции HTableFind

file:///home/plato/mipt_projs/hash_table/readme_images/opt1_2.png![изображение](https://user-images.githubusercontent.com/89589647/167072009-84fa1a27-f82c-4f5e-ba7e-36fd01da1406.png)

Перейдем к выводу all calees для самой затратной функции ListFind
file:///home/plato/mipt_projs/hash_table/readme_images/opt1_3.png![изображение](https://user-images.githubusercontent.com/89589647/167072066-ac155b2e-9876-4ba4-92d6-b1b14c4f0b06.png)

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

Ускорение программы еще на 
### оптимизация 3
Теперь все затраты идут на выполнение функции ListFind. Посмотрим, что же с ней не так. Для этого глянем на листинг.

![изображение](https://user-images.githubusercontent.com/89589647/165023932-0835c8e3-e60c-45b0-91c7-eb7b960ed626.png)

Посмотрим на асемблерный листинг 510-511 строчек.

![изображение](https://user-images.githubusercontent.com/89589647/165024012-c7b085bc-7421-47e3-b922-a068cba354c7.png)

Перепишем часть с циклом через ассемблерную вставку, чтобы уменьшить общее количество операций и также операций обращения к памяти. (Учитывая что мы писали fsctrcmp и знаем какие регистры там используются)

```nasm
.intel_syntax noprefix
mov rcx,  0			
mov ebx, size			
mov rsi, val		
mov r9,  cur_node		
cmp_loop:				
mov rdi, [r9]			
push rdi				
push rsi				
call fstrcmp			
pop rsi				
pop rdi				
cmp rax, 0				
je found_label			
add r9, 0x10			
inc rcx				
cmp ecx, ebx			
jne cmp_loop			
jmp found_label		
found_label:			
mov res, rax			
.att_syntax prefix		
```

![изображение](https://user-images.githubusercontent.com/89589647/165030621-0d22b4a5-ae43-4fc7-b388-e3902fa300bd.png)

Программа стала быстрее еще на 30%

### итог
Программа была ускорена в 2.4 раза.
Было написано 55 строчек на ассемблере, тем самым наша главная метрика равна

```
k = 2.4/55 * 1000 = 43.6
```

# Спектральный анализ

Проведём спектральный анализ нашей хэш таблицы на 6 хэш функциях для того, чтобы оценить их эффективность. Для этого зафиксируем размер таблицы.
file:///home/plato/mipt_projs/hash_table/readme_images/bars.png![изображение](https://user-images.githubusercontent.com/89589647/167064045-517d1c23-2a21-43c1-848c-c4252159a061.png)

Нетрудно заметить, что функции возврата константы, ascii первого символа строки и длины строки образуют один больший кластер, вследствие чего хэш функции являются чудовищно плохими. Контрольная сумма уже получше, хотя всё также образует один большой кластер вначале структуры. Rol hash еще лучше, значение дисперсии - 50.14. А победителем само собой является crc32 с дисперсией 6.1.
