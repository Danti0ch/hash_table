#ifndef HASH_FUNCS_H
#define HASH_FUNCS_H

typedef unsigned int uint;

const uint MAX_STR_LEN  = 10000;
const uint BITS_IN_BYTE = 8;

uint HashReturn0(const char* str);
uint HashFirstChar(const char* str);
uint HashCheckSum(const char* str);
uint HashStrLen(const char* str);
uint HashBRol(const char* str);

// TODO: rename
// TODO: добавить поле под статистику
struct hash_func_meta{

    uint (*p_func)(const char* str);
    const char* name;
    const char* descr;
    // uint statistic;
};

const hash_func_meta hash_funcs[] = {
    {
        HashReturn0,
        "Вовзрашает 0",
        "функция возвращает нулевое значение для любого ключа"
    },
    {
        HashFirstChar,
        "Ascii первого символа",
        "возвращает аски код первого символа строки"
    },
    {
        HashCheckSum,
        "Контрольная сумма",
        "Возвращает сумму ascii значений символов в строке"
    },
    {
        HashStrLen,
        "Длина строки",
        "Возвращает длину строки"
    },
    {
        HashBRol,
        "rol + xor",
        "индуктивное вычисление хэша: h_0 = str[0], h_<i+1> = rol(h_i) xor str[i+1]"
    }
};

const uint N_HASH_FUNCS = sizeof(hash_funcs) / sizeof(hash_func_meta);

#endif // HASH_FUNCS_H
