# hash_table
реализация хэш таблицы с закрытой адресацией

## Оптимизационная часть

### Оценка ускорения
Мы будем использовать профайлер callgrind, который показывает в процентах долю затрат каждой функции. Следовательно, если до оптимизации доля была x, а после неё доля стала y, до оптимизации было t единиц затрат программы, а стало t - delt, то имеет место равенство 
```
(1-x)*t = (1-y)*(t-delt).
```
Из чего получаем 
```
t/(t - delt) = (1-y)/(1-x).
```

Также мы имеем метрику оценки производительности с учётом количества ассемблерных вставок: 
```
N/M * 1000
```
Где N - общее ускорение программы, а M - количество ассемблерых строк.

### Оптимизации
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
Так как вычисления хэша на каждой итерации довольно трудоёмкий процесс в котором происходит обращение к памяти и несколько битовых операций, то имеет смысл немного векторизовать вычисления.
Воспользуемся simd функциями, конкретнее встроенной функцией, которая считает хэш по алгоритму crc32. Будем считать хэш не побайтово, а порциями по 4 байта.

![изображение](https://user-images.githubusercontent.com/89589647/164611592-27483d94-e5de-4bf5-9f3d-830e6b310505.png)

Ничего себе! Теперь получение хэша будет происходить почти мгновенно. Наша программа была ускорена на 50%.

### оптимизация 2

Следующим шагом будет оптимизация функции strcmp. Упростим функцию __strcmp_avx2 и полностью перепишем её на ассемблере 
![изображение](https://user-images.githubusercontent.com/89589647/164616008-1e01dd14-843a-4eb9-a92b-0550b60caf17.png)

Программа ускорилась уже на 110%.

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

Программа стала быстрее на 
![изображение](https://user-images.githubusercontent.com/89589647/165029541-33fe32e1-aa9d-42fc-802b-b8566cb9aed4.png)

# Спектральный анализ

Проведём спектральный анализ нашей хэш таблицы на 6 хэш функциях для того, чтобы оценить их эффективность. Для этого зафиксируем размер таблицы.
![изображение](https://user-images.githubusercontent.com/89589647/164673579-47ba0dd7-9912-4bb4-bac6-f08d89415ede.png)
Нетрудно заметить, что функции возврата константы, ascii первого символа строки и длины строки образуют один больший кластер, вследствие чего хэш функции являются чудовищно плохими. Контрольная сумма уже получше, хотя всё также образует один большой кластер вначале структуры. Rol hash еще лучше, значение дисперсии - 13.8. А победителем само собой является crc32 с дисперсией 4.89.
