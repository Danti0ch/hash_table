#include "hash_funcs.h"
#include <assert.h>
#include <stdlib.h>

//----------------------PUBLIC-FUNCTIONS-DEFINITIONS----------------------//

uint HashReturn0(const char* str, const uint mod){

    assert(str != NULL);

    return 0;
}
//----------------------------------------------------------------------------------------//

uint  HashFirstChar(const char* str, const uint mod){

    assert(str != NULL);

    return (uint)str[0] % mod;
}
//----------------------------------------------------------------------------------------//

uint HashCheckSum(const char* str, const uint mod){

    assert(str != NULL);

    uint sum = 0;
    for(uint i = 0; str[i] != 0, i < MAX_STR_LEN; i++){
        sum += (uint)str[i];
    }

    return sum % mod;
}
//----------------------------------------------------------------------------------------//

uint HashStrLen(const char* str, const uint mod){

    assert(str != NULL);

    uint len = 0;
    for(; str[len] != 0 && len < MAX_STR_LEN; len++){
        ;
    }

    return len % mod;
}
//----------------------------------------------------------------------------------------//

uint HashBRol(const char* str, const uint mod){

    assert(str != NULL);

    char hash = str[0];

    for(uint i = 1; str[i] != 0 && i < MAX_STR_LEN; i++){
        hash = (hash >> 1) | (hash << (sizeof(hash) * BITS_IN_BYTE - 1));

        hash ^= str[i];
    }

    return (uint)hash % mod;
}
//----------------------------------------------------------------------------------------//
