# hash_table
реализация хэш таблицы с закрытой адресацией

## Оптимизационная часть

### Оценка ускорения
Мы будем использовать профайлер callgrind, который показывает в процентах долю затрат каждой функции. Будем оценивать производительность нагрузочной функции с помощью функции rdtsc.

Также мы имеем метрику оценки производительности с учётом количества ассемблерных вставок: 

```
N/M * 1000
```
Где N - общее ускорение программы, а M - количество ассемблерых строк.

### Оптимизации
Идея юзкейса:  предварительная загрузка данных в таблицу, а потом операции нахождения и обновления элемента таблицы. Будем использовать таблицу так: предположим, что у нас есть очень большой файл A, который полностью поместиться в таблицу не сможет. Возьмем файл-словарь B, загрузим из него слова в таблицу и будем считать частоту появления слов из словаря B в большом файле A. То есть слова, которые присутствует в файле A, и отстутствуют в файле B добавляться в таблицу не будут. Будем оценивать производительность стадии подсчета и обновления частоты. Так как обновление не будет приводить к добавлению элементов, то не будут затрагиваться части HTableInsert, которых бы не было в HTableFind.

В качестве словаря B возьмем файл words.txt, а в качестве файла A - файл enwik8.

Размер хэш таблицы: 1000000

Флаг оптимизации: -O3

Оценку производительности хэш таблицы будем проводить с помощью профайлера callgrind и здравого смысла.

Нагрузочная функция будет иметь следующий вид:
```cpp
void LoadHTable(HTable* htable, const text_storage* text, const text_storage* dict, const size_t htable_size){

    int cur_freq = 0;
    
    for(uint n_word = 0; n_word < text->n_words; n_word++){

        const char* cur_word = GetWord(text, n_word);

        int isfind = HTableFind(htable, cur_word, &cur_freq);
        if(isfind){
            HTableInsert(htable, cur_word, cur_freq + 1);
        }
    }

    return;
}
```

Запустим программу на тестовых данных и посмотрим, какую информацию нам выдаст профайлер. Посмотрим нагрузку каждой функции в процентном соотношении от функции LoadHTable:
![изображение](https://user-images.githubusercontent.com/89589647/167430890-67870e2f-cecf-4ea7-8983-bff3e2552080.png)

Также был сделан замер по результатам которого на выполнение функции LoadHTable ушло 4644596385 тактов процессора(медиана из 64 тестов).

### оптимизация 1

Нетрудно заметить, что функция HTableFind тратит больше всего ресурсов. Посмотрим какой конкретно участок кода потребляет больше всего ресурсов

![изображение](https://user-images.githubusercontent.com/89589647/167431066-d1793b6a-6220-4f12-a9e6-fc3f3c3475e4.png)

Оказывается это хэш функция. Заметим, также, что хэш функция и функция верификация хэш таблицы являются inline. Линейное вычисление хэша в совокупности с частым обращением к памяти приводит к тому, что хэш функция тратит много ресурсов. Воспользуемся тем, что строки в таблице ограничены длиной 32 и будем вычислять хэш функцию через simd инструкцию вычисления crc32. Будем считать сразу по 32 бита.

```cpp
inline uint get_hash(const char* str){

    uint hash = 0xFFFFFFFF;

	for(uint ind = 0; ind < 8; ind += 4){
	    uint hash_val = *((uint*)(str + ind));
	    if(hash_val == 0) break;
	    hash = _mm_crc32_u32(hash, hash_val);
	}
    
    return hash;
}

```

Теперь на выполнение LoadHTable уходит 3907377045 тактов. Ускорение на 18%.

### оптимизация 2

Посмотрим теперь на вывод профайлера:
![изображение](https://user-images.githubusercontent.com/89589647/167431469-59f429b9-a0e2-4640-af15-c1affd4c7bde.png)

ListFind - функция, которая употребляется в функции вставки и поиска и суммарно тратит около 39% всех затрат. Посмотрим на её листинг

![изображение](https://user-images.githubusercontent.com/89589647/167431791-f6443d2c-8b18-4281-80cf-ddee7c1b51a4.png)

На strcmp уходит около половины ресурсов при выполнении функции ListFind. Попробуем её прооптимизировать. Опять же воспользуемся тем, что строки в таблице содержатся в ячейках по 32 байта. Воспользуемся inline asm чтобы через ymm регистры выполнять сравнение за O(const), а не за O(n). Напишем для этого отдельную функцию ListFindAligned.

```cpp
list_T* _ListFindAligned(const list* obj, char* temp, const char* key, const size_t key_len, META_PARAMS){
	
	LIST_OK(obj)

	node* cur_node = obj->nodes + obj->head;
	
	memset(temp, 0, ALIGN_RATIO);
	memcpy(temp, key, key_len);

	for(uint n_node = 0; n_node < obj->size; n_node++){

		uint res = 0;
		
		asm(
			".intel_syntax noprefix			\n\t"
			
			"vmovdqa ymm0, [%1]				\n\t"
			"vmovdqa ymm1, [%2]				\n\t"
			// vpcmpeqq
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
}
```

Также предоставим пользователю возможность передавать в функцию уже выровненные строки, что избавит нас от нужды делать копирование в выравненное временное хранилище. Для этого была реализована функция HTableFindAligned, HTableInsertAligned и ListFindAlignedByAligned.

```cpp

list_T* _ListFindAlignedByAligned(const list* obj, const char* key, META_PARAMS){
	
	LIST_OK(obj)
	const uint size = obj->size;

	uint res = 0;

	#if ENABLE_SORT
		node* cur_node = obj->nodes;
	#else
		node* cur_node = obj->nodes + obj->head;
	#endif // ENABLE_SORT

	for(uint n_node = 0; n_node < size; n_node++){
		
		asm(
			".intel_syntax noprefix			\n\t"
			
			"vmovdqa ymm0, [%1]				\n\t"
			"vmovdqa ymm1, [%2]				\n\t"
			// vpcmpeqq
			"vpcmpeqb ymm2, ymm0, ymm1		\n\t"

			"vpmovmskb eax, ymm2			\n\t"
			"mov %0, eax					\n\t"

			".att_syntax prefix				\n\t"

			:"=r"(res)
			:"r"(cur_node[n_node].val.p_key), "r"(key)
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

В дальнейшем мы будем оптимизировать именно эти функции

Генеральная функция выполняется уже за 3186492375 тактов
Ускорение программы еще на 27%

### оптимизация 3
![изображение](https://user-images.githubusercontent.com/89589647/167433242-b67f5e11-d6cd-460f-9ec4-29977df72bdc.png)

Посмотрим, могут ли быть еще какие-то оптимизации. Рассмотрим функцию HTableFindAligned

![изображение](https://user-images.githubusercontent.com/89589647/167433440-15b68fc8-ea17-4d2a-8106-22960a626b18.png)
![изображение](https://user-images.githubusercontent.com/89589647/167433494-6b61638a-def5-46cc-9c3c-920fe73a5c37.png)

Исключая функцию ListFindAlignedByAligned все операции в функции являются примитивными и необходимыми, незатратными или уже оптимизированными.

![изображение](https://user-images.githubusercontent.com/89589647/167433717-e9d7c10e-5b54-4f3f-8512-f0acfa94577d.png)
![изображение](https://user-images.githubusercontent.com/89589647/167433739-f8703934-7038-4fcc-a53f-4b11113d5be8.png)

С ней всё то-же самое.

Рассмотрим теперь ListFindAlignedByAligned

![изображение](https://user-images.githubusercontent.com/89589647/167433860-9969e13d-ebb2-4816-bc59-07396de472ca.png)

Посмотреть на асм листинг нашей функции. Цикл тратит подозрительно много ресурсов, чтобы посмотреть в чем дело, посмотрим на асм листинг нашей функции.

![изображение](https://user-images.githubusercontent.com/89589647/167434072-c7040b1e-3c2d-4ab7-83f3-790860fc77b1.png)

Операции, связанные с циклом указанны в строках 40194B-401967. В них происходит проверка размера списка, Расчёт адреса последнего элемента в списке и операции сравнения, увеличения счётчика. Заметим, что наша хэш функция довольно хорошая, около 80% всех списков ненулевой длины - единичной длины.

Конкретно:
```
292233 строк единичной длины
80475  строк большей длины
```

Тем самым можем избавиться от выполнения инструкций, связанных с циклом, просто выполнив код тела цикла один раз еще до самого цикла.

```cpp
#define fast_strcmp(n_node)							\
										\
asm(										\
			".intel_syntax noprefix			\n\t"		\
										\
			"vmovdqa ymm0, [%1]				\n\t"	\
			"vmovdqa ymm1, [%2]				\n\t"	\
										\
			"vpcmpeqb ymm2, ymm0, ymm1		\n\t"		\
										\
			"vpmovmskb eax, ymm2			\n\t"		\
			"mov %0, eax					\n\t"	\
										\
			".att_syntax prefix				\n\t"	\
										\
			:"=r"(res)						\
			:"r"(obj->nodes[0].val.p_key), "r"(key)			\
			:"ymm0", "ymm1", "ymm2", "rax"				\
		);

list_T* _ListFindAlignedByAligned(const list* obj, const char* key, META_PARAMS){
	
	LIST_OK(obj)
	const uint size = obj->size;

	uint res = 0;

	fast_strcmp(0)
			
	if(res == 0xFFFFFFFF) return &(obj->nodes[0].val);
	if(size == 1) return NULL;

	node* cur_node = obj->nodes;

	for(uint n_node = 1; n_node < size; n_node++){
		
		fast_strcmp(n_node)
		if(res == 0xFFFFFFFF) return &(cur_node[n_node].val);
	}

	return NULL;
}
```

Генеральная функция выполняется уже за 3131261610 тактов
Увеличение производительности на 3%

### итог
Дальнейшие оптимизации не требуются. Во первых последняя оптимизация дала нам немного - 3%. Необходимые примитивные инструкции убрать нельзя, а самые затратные участки кода уже были оптимизированы.

Тем самым мы получили ускорение:
```
4644596385 -> 3131261610 тактов
Или ускорение на 48%(в 1.483 раз)
```
<!---
### доп оптимизация
 Эта часть не относится к задаче, является просто оптимизации, убранной изза ненадобности в юзкейсе.
 
 Займемся оптимизацией функцией поиска и вставки невыровненного ключа.
 
![изображение](https://user-images.githubusercontent.com/89589647/167197403-76bcce65-4b15-4f9b-b2cb-0f6be0ce3280.png)

При вызове HTableFind почти всегда происходит вызов ListFindALigned и всегда в начале функций происходит расчёт длины ключа через функцию strlen.

![изображение](https://user-images.githubusercontent.com/89589647/167197639-9f9d3ea0-e1cc-404b-904f-9cb1c5c798ba.png)

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

LoadHTable выполняется теперь за 3610105673 тактов. Производительность выросла еще на 12%

### итог
Под наш юзкейс программа была ускорена на 31%.
Было написано 6 строчек на ассемблере, тем самым наша главная метрика равна

```
k = 1.43/6 * 1000 = 238
```
-->
# Спектральный анализ

Проведём спектральный анализ нашей хэш таблицы на 6 хэш функциях для того, чтобы оценить их эффективность. Для этого зафиксируем размер таблицы.
![изображение](https://user-images.githubusercontent.com/89589647/167064045-517d1c23-2a21-43c1-848c-c4252159a061.png)

Нетрудно заметить, что функции возврата константы, ascii первого символа строки и длины строки образуют один больший кластер, вследствие чего хэш функции являются чудовищно плохими. Контрольная сумма уже получше, хотя всё также образует один большой кластер вначале структуры. Rol hash еще лучше, значение дисперсии - 50.14. А победителем само собой является crc32 с дисперсией 6.1.
