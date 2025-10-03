#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
#include <malloc/malloc.h> // Системно зависимая библиотека, только для macos
#else
#error
#endif

#include <limits.h>
#include "policy.h"
#include "stack_.h"
#include "io_utils.h"

#define begin do
#define end while(0)

// int typedef stack_element_t;

// struct my_stack {
//     canary_protection(int              canary1;)
//
//     size_t              size;
//     size_t              capacity;
//     stack_element_t *   data;
//
//     hash_protection(size_t           hash;)
//     hash_protection(size_t           data_hash;)
//
//     canary_protection(int              canary2;)
// };

// enum {
//     SUCCESS                          = 0,
//     NULL_PTR_PASSED                  = 1,
//     CANNOT_ALLOCATE_MEMORY           = 2,
//     STACK_EMPTY                      = 3,
//     STACK_DATA_IS_NULL_PTR           = 4,
//     SIZE_BIGGER_THAN_CAPACITY        = 5,
//     STACK_OVERFLOW                   = 6,
//     DATABUF_SIZE_NOT_MATCH_CAPACITY  = 7,
//     CORRUPT_POISON                   = 8,
//     POISON_COLLISION                 = 9,
//     WRONG_REALLOC_SIZE               = 10,
//     canary_protection(CORRUPT_CANARY = 11,)
//     hash_protection(CORRUPT_HASH     = 12,)
//     CANNOT_REALLOCATE_MEMORY         = 13,
// } typedef STACK_ERRNO;

canary_protection(
#define POISON  (stack_element_t) 0x96716f66
#define CANARY1 (stack_element_t) 0xDED0AFFF
#define CANARY2 (stack_element_t) 0xEDA0FAAA
#define CANARY3 (stack_element_t) 0xDED00EDA
#define CANARY4 (stack_element_t) 0xABCD1234
)

#define StackValidateReturnIfErr(stack_var_name)                                           \
    {                                                                           \
        STACK_ERRNO stk_errno = StackValidatorI(stack_var_name);                 \
        if (stk_errno != STACK_ERRNO::SUCCESS) {                               \
            StackDumpI(stack_var_name, stk_errno, "stk_errno is not sucssess");  \
            return stk_errno;                                                   \
        }                                                                       \
    };

/*
this algorithm was created for sdbm (a public-domain reimplementation of ndbm) database library.
it was found to do well in scrambling bits, causing better distribution of the keys and fewer splits.
it also happens to be a good general hashing function with good distribution.
the actual function is hash(i) = hash(i - 1) * 65599 + str[i];
what is included below is the faster version used in gawk.
[there is even a faster, duff-device version]
the magic prime constant 65599 (2^6 + 2^16 - 1) was picked out of thin air
while experimenting with many different constants. this is one of the algorithms
used in berkeley db (see sleepycat) and elsewhere.
*/
static unsigned long long sdbm(const void * const void_data, size_t max_len) {
    const char * data = (const char *) void_data;
    unsigned long long hash = 0;
    // printf("%s\n", data);
    while (max_len-- > 0) {
        char c = *data++;
        // printf("%max_len = %zu\t", max_len);
        // printf("hash = %lld\t", hash);
        // printf("c = %d\n", c);
        hash = (unsigned long long) c + (hash << 16) + (hash << 6) - hash;
    }
    return hash;
}

// Нахождение ближайшей степени 2 >= x
static size_t cpl2(size_t x) {
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x+1;
}

static STACK_ERRNO StackValidatorI(my_stack * const stk) {
    if (stk == NULL) // Стек - nullptr
        return STACK_ERRNO::NULL_PTR_PASSED;
    canary_protection(
        if (stk->canary1 != CANARY1 || stk->canary2 != CANARY2){ // Одна из канареек в структуре была испорчена
            return STACK_ERRNO::CORRUPT_CANARY;
        }
    )
    if (stk->data == NULL) // Динамический массив - не создан
    return STACK_ERRNO::STACK_DATA_IS_NULL_PTR;

    if (stk->size > stk->capacity) // Количество элементов в стеке больше чем его вместимость
    return STACK_ERRNO::SIZE_BIGGER_THAN_CAPACITY;

    size_t total_slots = malloc_size(stk->data) / sizeof(stack_element_t);
    if (total_slots < stk->capacity) // Размер динамической памяти меньше вместимости
    return STACK_ERRNO::DATABUF_SIZE_NOT_MATCH_CAPACITY;

    canary_protection(
        if (stk->data[0] != CANARY3 || stk->data[stk->capacity + 1] != CANARY4) {
            printf(BRIGHT_RED("MEOW\n"));
            StackDumpI(stk, CORRUPT_CANARY, "CORRUPT_CANARY in validator");
            // Одна из канареек в массиве была испорчена
            return STACK_ERRNO::CORRUPT_CANARY;
        }
    )
    for (size_t i = stk->size canary_protection(+1); i < stk->capacity - 1 canary_protection(+1); ++i) {
        if (stk->data[i] != POISON)
            return STACK_ERRNO::CORRUPT_POISON;
    }
    hash_protection(
        if (stk->data_hash != sdbm(stk->data, stk->capacity)) {
            // Хеш массива не совпадает с сохраненным хешем массива
            return STACK_ERRNO::CORRUPT_HASH;
        }

        uint64_t old_hash = stk->hash;
        stk->hash = 0;
        uint64_t now_hash = sdbm(stk, sizeof(my_stack));
        if (now_hash != old_hash) {
            // printf("now_hash is " BRIGHT_WHITE("%lld") " [%#x] old_hash is " BRIGHT_WHITE("%lld") " [%#x]\n", now_hash, now_hash, old_hash, old_hash);
            stk->hash = old_hash;
            return STACK_ERRNO::CORRUPT_HASH;
        }
        stk->hash = old_hash;
    )
    return STACK_ERRNO::SUCCESS;
}

my_stack * StackCtorI(size_t capacity, STACK_ERRNO * stk_errno) {
    if (stk_errno == NULL)
        return NULL;

    my_stack * stk = (my_stack *) calloc(1, sizeof(my_stack));
    if (stk == NULL) {
        *stk_errno = STACK_ERRNO::CANNOT_ALLOCATE_MEMORY;
        return NULL;
    }

    canary_protection(
        stk->canary1 = CANARY1;
        stk->canary2 = CANARY2;
    )

    capacity = cpl2(capacity);
    stk->capacity = capacity;
    canary_protection(capacity += 2;) // Добавляем место для canary

    stk->data = (stack_element_t *) calloc(capacity, sizeof(stack_element_t));
    if (stk->data == NULL) {
        *stk_errno = STACK_ERRNO::CANNOT_ALLOCATE_MEMORY;
        return NULL;
    }

    for (size_t i = stk->size; i < capacity; ++i) {
        stk->data[i] = POISON;
    }

    canary_protection(
        stk->data[0] = CANARY3;
        stk->data[capacity - 1] = CANARY4;
    )

    hash_protection(
        stk->data_hash = sdbm(stk->data, stk->capacity);

        stk->hash = 0;
        stk->hash = sdbm(stk, sizeof(my_stack));
    )

    *stk_errno = StackValidatorI(stk);
    StackDumpI(stk, SUCCESS, "validator check");
    if (*stk_errno != STACK_ERRNO::SUCCESS) {
        free(stk);
        return NULL;
    }

    return stk;
}

STACK_ERRNO StackPushI(my_stack * const stk, stack_element_t value) {
    StackValidateReturnIfErr(stk);

    DEBUG_PRINT("stk ptr in %s is [%p]\n", __PRETTY_FUNCTION__, stk);

    if (value == POISON) {
        StackDumpI(stk, STACK_ERRNO::POISON_COLLISION, StackErrorI(STACK_ERRNO::POISON_COLLISION));
        return STACK_ERRNO::POISON_COLLISION;
    }
    if (stk->size + 1 > stk->capacity) {
        // StackDumpI(stk, STACK_ERRNO::STACK_OVERFLOW, StackErrorI(STACK_ERRNO::STACK_OVERFLOW));
        // return STACK_OVERFLOW;
        StackReallocI(stk, stk->capacity*2);
    }
    stk->data[stk->size++ canary_protection(+1)] = value;

    hash_protection(
        stk->data_hash = sdbm(stk->data, stk->capacity);

        stk->hash = 0;
        stk->hash = sdbm(stk, sizeof(my_stack));
    )

    StackValidateReturnIfErr(stk);
    return STACK_ERRNO::SUCCESS;
}

STACK_ERRNO StackPopI(my_stack * const stk, stack_element_t * value) {
    StackValidateReturnIfErr(stk);
    if (value == NULL)
        return STACK_ERRNO::NULL_PTR_PASSED;

    if (stk->size == 0 canary_protection(+ 1))
        return STACK_ERRNO::STACK_EMPTY;
    *value = stk->data[--stk->size canary_protection(+ 1)];

    stk->data[stk->size canary_protection(+ 1)] = POISON;

    hash_protection(
        stk->data_hash = sdbm(stk->data, stk->capacity);

        stk->hash = 0;
        stk->hash = sdbm(stk, sizeof(my_stack));
    )

    StackValidateReturnIfErr(stk);
    return STACK_ERRNO::SUCCESS;
}

STACK_ERRNO StackReallocI(my_stack * const stk, size_t new_size) {
    StackValidateReturnIfErr(stk);

    if (new_size < stk->size canary_protection(+ 1)) {
        return STACK_ERRNO::WRONG_REALLOC_SIZE;
    }

    new_size = new_size canary_protection(+ 2); // Если включены канарейки то размер на 2 больше

    if (new_size == stk->capacity canary_protection(+ 2)) { // Не изменяем стек
        StackValidateReturnIfErr(stk);
        return STACK_ERRNO::SUCCESS;

    } else if (new_size < stk->capacity) { // Сжимаем стек
        stack_element_t * new_data = (stack_element_t *) realloc(stk->data, new_size * sizeof(stack_element_t));
        if (new_data == NULL)
            return STACK_ERRNO::CANNOT_REALLOCATE_MEMORY; // Если реаллок не выполнился, он не изменяет старый указатель
        stk->data = new_data;
        stk->capacity = new_size canary_protection(- 2); // В структуре хранится размер без учета канареек
        canary_protection(new_data[new_size - 1] = CANARY4);

    } else if ((new_size > stk->capacity)) { // расширяем стек
        stack_element_t * new_data = (stack_element_t *) realloc(stk->data, new_size * sizeof(stack_element_t));
        if (new_data == NULL)
            return STACK_ERRNO::CANNOT_REALLOCATE_MEMORY; // Если реаллок не выполнился, он не изменяет старый указатель

        stk->data = new_data;

        // Инициализируем новую память ядом, переносим канарейку (если есть) в конец
        for (size_t i = stk->capacity canary_protection(+ 1); i < new_size canary_protection(- 1); ++i) {
            new_data[i] = POISON; // Указатель на тот же массив, но каждый раз не получаем его из структуры
        }
        canary_protection(new_data[new_size - 1] = CANARY4);

        stk->capacity = new_size canary_protection(- 2); // В структуре хранится размер без учета канареек
    }

    hash_protection( // Пересчитываем хэш после реаллокации
        stk->data_hash = sdbm(stk->data, stk->capacity);

        stk->hash = 0;
        stk->hash = sdbm(stk, sizeof(my_stack));
    )

    StackValidateReturnIfErr(stk);
    return STACK_ERRNO::SUCCESS;
}

STACK_ERRNO StackDtorI(my_stack * stk) {
    StackValidateReturnIfErr(stk);
    // TODO: засрать стек

    size_t data_bytes  = malloc_size(stk->data);
    size_t stack_bytes = malloc_size(stk);

    memset(stk->data, 0xDD, data_bytes);
    free(stk->data);
    stk->data = NULL;

    memset(stk, 0xDD, stack_bytes);
    free(stk);
    stk = NULL;

    return STACK_ERRNO::SUCCESS;
}

const char * StackErrorI(STACK_ERRNO stk_errno) {
    switch (stk_errno) {
        case STACK_ERRNO::SUCCESS:                          return "No problems with stack";
        case STACK_ERRNO::NULL_PTR_PASSED:                  return "Stack pointer is NULL";
        case STACK_ERRNO::CANNOT_ALLOCATE_MEMORY:           return "Failed to allocate memory for stack data";
        case STACK_ERRNO::STACK_EMPTY:                      return "Attempt to pop from empty stack";
        case STACK_ERRNO::STACK_DATA_IS_NULL_PTR:           return "Stack data pointer is NULL";
        case STACK_ERRNO::SIZE_BIGGER_THAN_CAPACITY:        return "Stack size exceeds capacity (corruption?)";
        case STACK_ERRNO::STACK_OVERFLOW:                   return "Trying to push into filled stack";
        case STACK_ERRNO::DATABUF_SIZE_NOT_MATCH_CAPACITY:  return "Size of databuf malloc section dont't match capacity of stack";
        case STACK_ERRNO::CORRUPT_POISON:                   return "Not Poison in empty part => stack is damaged";
        case STACK_ERRNO::POISON_COLLISION:                 return "Trying to insert poison. Use another value";
        case STACK_ERRNO::WRONG_REALLOC_SIZE:               return "Size to realloc must be bigger then size of stack";
        canary_protection(case STACK_ERRNO::CORRUPT_CANARY:                return "Canary is spoiled => stack is damaged";)
        hash_protection(case STACK_ERRNO::CORRUPT_HASH:                  return "Hash is spoiled => stack is damaged";)
        case STACK_ERRNO::CANNOT_REALLOCATE_MEMORY:         return "Realloc returned NULL PTR, this error is not fatal, stack data was not deleted or freed";
        default:                                            return "Unknown stack error";
    }
}

void StackDumpI_impl(my_stack * const stk, STACK_ERRNO stk_errno, const char * const reason,
        const char * file, int line, const char * func) {
    assert(reason   != NULL);
    assert(file     != NULL);
    assert(func     != NULL);

    printf("================== " BRIGHT_WHITE("STACK DUMP") " ==================\n");
    printf("called from " BRIGHT_BLUE("%s:%d") " in function " BRIGHT_BLUE("%s") "\n", file, line, func);

    // Проверка на NULL
    if (stk == NULL) {
        printf(RED("ERROR: Stack pointer is NULL!\n"));
        printf("================================================\n\n");
        return;
    }

    // Состояние стека
    if (stk_errno != STACK_ERRNO::SUCCESS) {
        printf(BOLD(BRIGHT_RED("STACK IS IN INVALID STATE\n")));
        printf("errno is " RED("%d") " → " BRIGHT_RED("%s") "\n", stk_errno, StackErrorI(stk_errno));
    } else {
            printf(BRIGHT_GREEN("STACK IS VALID\n"));
    }

    printf("reason: " BRIGHT_BLUE("%s\n"), reason);
    printf("stack at " CYAN("%p\n"), stk);

    size_t actual_bytes = 0;
    size_t expected_bytes = (stk->capacity canary_protection(+ 2)) * sizeof(stack_element_t);

    if (stk->data != NULL) {
        actual_bytes = malloc_size(stk->data);
    }
    canary_protection(
        printf("\tleft_canary = " BRIGHT_GREEN("%d") " [%#x] " BRIGHT_GREEN("(CANARY)\n"), stk->canary1, (unsigned) stk->canary1);
    )
    // Вывод capacity
    if (stk->data != NULL && actual_bytes != expected_bytes) {
        printf("\tcapacity    = " BRIGHT_WHITE("%zu") " (expected " CYAN("%zu bytes") ")\n", stk->capacity, expected_bytes);
    } else {
        printf("\tcapacity    = " BRIGHT_WHITE("%zu") "\n", stk->capacity);
    }

    printf("\tsize        = " BRIGHT_WHITE("%zu") " (last element index = ", stk->size);
    if (stk->size == 0) {
        printf(BRIGHT_RED("NONE"));
    } else {
        printf(BRIGHT_YELLOW("%zu"), stk->size - 1 canary_protection(+ 1));
    }
    printf(")\n");
    hash_protection(
        printf("\thash        = " BRIGHT_WHITE("%zu") " [%#zx]\n", stk->hash, stk->hash);
        printf("\tdata_hash   = " BRIGHT_WHITE("%zu") " [%#zx]\n", stk->data_hash, stk->data_hash);
    )

    // Вывод data
    if (stk->data == NULL canary_protection(
        && stk->data[0] == CANARY3 && stk->data[stk->capacity + 1] == CANARY4)) { // TODO проверить канарейки
            printf("\tdata[" RED("NULL") "] at " CYAN("%p") "\n", stk->data);
        } else {
        printf("\tdata[" CYAN("%zu actual bytes") "] at " CYAN("%p") "\n", actual_bytes, stk->data);

        if (actual_bytes > expected_bytes) {
            printf("\t" YELLOW("Note: allocator allocated extra %zu bytes\n"), actual_bytes - expected_bytes);
        }
    }

    printf("\t{\n");

    if (stk->data == NULL) {
        printf("\t\t" BRIGHT_RED("DATA IS NULL POINTER!\n"));
    } else {
        size_t total_slots = actual_bytes / sizeof(stack_element_t);
        canary_protection(printf("\t\t" YELLOW(" ") BRIGHT_GREEN("[0]") " = " BRIGHT_GREEN("%d") " [%#x] " BRIGHT_GREEN("(CANARY)\n"), stk->data[0], (unsigned) stk->data[0]);)
        for(size_t i = 0 canary_protection(+ 1); i < total_slots; ++i){
            if (i < stk->size canary_protection(+ 1)) {
                printf("\t\t" BRIGHT_MAGENTA("*") BRIGHT_YELLOW("[%zu]") " = " BRIGHT_WHITE("%d") " [%#x]\n", i, stk->data[i], (unsigned) stk->data[i]);
            } else if (i < stk->capacity canary_protection(+ 1)) {
                printf("\t\t" YELLOW(" ") YELLOW("[%zu]") " = " YELLOW("%d") " [%#x] " BRIGHT_BLACK("(POISON)\n"), i, stk->data[i], (unsigned) stk->data[i]);
            } else if (i == stk->capacity canary_protection(+ 1)) {
                canary_protection(printf("\t\t" YELLOW(" ") BRIGHT_GREEN("[%zu]") " = " BRIGHT_GREEN("%d") " [%#x] " BRIGHT_GREEN("(CANARY)\n"), i, stk->data[i], (unsigned) stk->data[i]);)
            } else {
                printf("\t\t" BRIGHT_BLACK(" ") BRIGHT_BLACK("[%zu]") " = " BRIGHT_BLACK("%d") " [%#x] " BRIGHT_BLACK("(padding)\n"), i, stk->data[i], (unsigned) stk->data[i]);
            }
        }
    }

    printf("\t}\n");
    canary_protection(printf("\tright_canary = " BRIGHT_GREEN("%d")" [%#x] " BRIGHT_GREEN("(CANARY)\n"), stk->canary2, (unsigned) stk->canary2);)
    printf("}\n");
    printf("================================================\n\n");
}