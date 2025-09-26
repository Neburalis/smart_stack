#include <assert.h>
#include <stdio.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#error
#endif
#include <limits.h>
#include "stack.h"
#include "io_utils.h"

#define begin do
#define end while(0)

// int typedef stack_element_t;
// TODO - условная компиляция

struct my_stack {
    int                 canary1;
    size_t              size;
    size_t              capacity;
    size_t              hash;
    size_t              data_hash;
    stack_element_t *   data;
    int                 canary2;
};

// enum {
//     SUCSSESS                        = 0,
//     NULL_PTR_PASSED                 = 1,
//     CANNOT_ALLOCATE_MEMORY          = 2,
//     STACK_EMPTY                     = 3,
//     STACK_DATA_IS_NULL_PTR          = 4,
//     SIZE_BIGGER_THAN_CAPACITY       = 5,
//     STACK_OVERFLOW                  = 6,
//     DATABUF_SIZE_NOT_MATCH_CAPACITY = 7,
//     CORRUPT_POISON                  = 8,
//     POISON_COLLISION                = 9,
//     CORRUPT_CANARY                  = 10,
//     CORRUPT_HASH                    = 11,
// } typedef STACK_ERRNO;

#define POISON  (stack_element_t) 0x96716f66
#define CANARY1 (stack_element_t) 0xDED0AFFF
#define CANARY2 (stack_element_t) 0xEDA0FAAA
#define CANARY3 (stack_element_t) 0xDED00EDA
#define CANARY4 (stack_element_t) 0xABCD1234

#define StackValidate()                                             \
    begin {                                                         \
        STACK_ERRNO stk_errno = StackValidator(stk);                \
        if (stk_errno != STACK_ERRNO::SUCSSESS) {                   \
            StackDump(stk, stk_errno, "stk_errno is not sucssess"); \
            return stk_errno;                                       \
        }                                                           \
    } end;

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
unsigned long long sdbm(void * void_data, size_t max_len) {
    char * data = (char *) void_data;
    unsigned long long hash = 0;
    // printf("%s\n", data);
    while (max_len-- > 0) {
        char c = *data++;
        // printf("%max_len = %zu\t", max_len);
        // printf("hash = %lld\t", hash);
        // printf("c = %d\n", c);
        hash += c;
    }
    return hash;
}

// Нахождение ближайшей степени 2 >= x
size_t cpl2(size_t x) {
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x+1;
}

my_stack_t * StackCtor(size_t capacity, STACK_ERRNO * stk_errno) {
    if (stk_errno == NULL)
        return NULL;

    my_stack_t * stk = (my_stack_t *) calloc(1, sizeof(my_stack_t));
    if (stk == NULL) {
        *stk_errno = STACK_ERRNO::CANNOT_ALLOCATE_MEMORY;
        return NULL;
    }

    stk->canary1 = CANARY1;
    stk->canary2 = CANARY2;

    capacity += 2; // Добавляем место для canary
    capacity = cpl2(capacity);
    stk->capacity = capacity;

    stk->data = (stack_element_t *) calloc(capacity, sizeof(stack_element_t));
    if (stk->data == NULL) {
        *stk_errno = STACK_ERRNO::CANNOT_ALLOCATE_MEMORY;
        return NULL;
    }
    stk->size = 1; // Потому что в 0 лежит canary

    size_t total_slots = malloc_size(stk->data) / sizeof(stack_element_t);
    for (size_t i = 1; i < total_slots - 1; ++i) {
        // calloc может выделить больше памяти чем попросили. Заполняем poison'ом ВСЕ
        stk->data[i] = POISON;
    }

    stk->data[0] = CANARY3;
    stk->data[capacity - 1] = CANARY4;

    stk->data_hash = sdbm(stk->data, stk->capacity);

    stk->hash = 0;
    stk->hash = sdbm(stk, sizeof(my_stack_t));

    *stk_errno = StackValidator(stk);
    if (*stk_errno != STACK_ERRNO::SUCSSESS) {
        free(stk);
        return NULL;
    }

    return stk;
}

STACK_ERRNO StackPush(my_stack_t * const stk, stack_element_t value) {
    StackValidate();

    if (value == POISON) {
        StackDump(stk, STACK_ERRNO::POISON_COLLISION, StackError(STACK_ERRNO::POISON_COLLISION));
        return STACK_ERRNO::POISON_COLLISION;
    }
    if (stk->size + 1 > stk->capacity) {
        StackDump(stk, STACK_ERRNO::STACK_OVERFLOW, StackError(STACK_ERRNO::STACK_OVERFLOW));
        return STACK_OVERFLOW;
    }
    stk->data[stk->size++] = value;

    stk->data_hash = sdbm(stk->data, stk->capacity);

    stk->hash = 0;
    stk->hash = sdbm(stk, sizeof(my_stack_t));

    StackValidate();
    return STACK_ERRNO::SUCSSESS;
}

STACK_ERRNO StackPop(my_stack_t * const stk, stack_element_t * value) {
    StackValidate();
    if (value == NULL)
        return STACK_ERRNO::NULL_PTR_PASSED;

    if (stk->size == 1)
        return STACK_ERRNO::STACK_EMPTY;
    *value = stk->data[--stk->size];

    stk->data[stk->size] = POISON;

    stk->data_hash = sdbm(stk->data, stk->capacity);

    stk->hash = 0;
    stk->hash = sdbm(stk, sizeof(my_stack_t));

    StackValidate();
    return STACK_ERRNO::SUCSSESS;
}

STACK_ERRNO StackDtor(my_stack_t * stk) {
    StackValidate();

    free(stk->data);
    free(stk);

    return STACK_ERRNO::SUCSSESS;
}

const char * StackError(STACK_ERRNO stk_errno) {
    switch (stk_errno) {
        case STACK_ERRNO::SUCSSESS:                         return "No problems with stack";
        case STACK_ERRNO::NULL_PTR_PASSED:                  return "Stack pointer is NULL";
        case STACK_ERRNO::CANNOT_ALLOCATE_MEMORY:           return "Failed to allocate memory for stack data";
        case STACK_ERRNO::STACK_EMPTY:                      return "Attempt to pop from empty stack";
        case STACK_ERRNO::STACK_DATA_IS_NULL_PTR:           return "Stack data pointer is NULL";
        case STACK_ERRNO::SIZE_BIGGER_THAN_CAPACITY:        return "Stack size exceeds capacity (corruption?)";
        case STACK_ERRNO::STACK_OVERFLOW:                   return "Trying to push into filled stack";
        case STACK_ERRNO::DATABUF_SIZE_NOT_MATCH_CAPACITY:  return "Size of databuf malloc section dont't match capacity of stack";
        case STACK_ERRNO::CORRUPT_POISON:                   return "Not Poison in empty part => stack is damaged";
        case STACK_ERRNO::POISON_COLLISION:                 return "Trying to insert poison. Use another value";
        case STACK_ERRNO::CORRUPT_CANARY:                   return "Canary is spoiled => stack is damaged";
        case STACK_ERRNO::CORRUPT_HASH:                     return "Hash is spoiled => stack is damaged";
        default:                                            return "Unknown stack error";
    }
}

STACK_ERRNO StackValidator(my_stack_t * const stk) {
    if (stk == NULL) // Стек - nullptr
        return STACK_ERRNO::NULL_PTR_PASSED;
    if (stk->canary1 != CANARY1 || stk->canary2 != CANARY2) // Одна из канареек в структуре была испорчена
        return STACK_ERRNO::CORRUPT_CANARY;
    if (stk->data == NULL) // Динамический массив - не создан
        return STACK_ERRNO::STACK_DATA_IS_NULL_PTR;
    if (stk->size > stk->capacity) // Количество элементов в стеке больше чем его вместимость
        return STACK_ERRNO::SIZE_BIGGER_THAN_CAPACITY;
    size_t total_slots = malloc_size(stk->data) / sizeof(stack_element_t);
    if (total_slots < stk->capacity) // Размер динамической памяти меньше вместимости
        return STACK_ERRNO::DATABUF_SIZE_NOT_MATCH_CAPACITY;
    if (stk->data[0] != CANARY3 || stk->data[stk->capacity - 1] != CANARY4) {
        // Одна из канареек в массиве была испорчена
        return STACK_ERRNO::CORRUPT_CANARY;
    }
    for (size_t i = stk->size; i < stk->capacity - 1; ++i) {
        if (stk->data[i] != POISON)
            return STACK_ERRNO::CORRUPT_POISON;
    }
    if (stk->data_hash != sdbm(stk->data, stk->capacity)) {
        // Хеш массива не совпадает с сохраненным хешем массива
        return STACK_ERRNO::CORRUPT_HASH;
    }

    uint64_t old_hash = stk->hash;
    stk->hash = 0;
    uint64_t now_hash = sdbm(stk, sizeof(my_stack_t));
    if (now_hash != old_hash) {
        // printf("now_hash is " BRIGHT_WHITE("%lld") " [%#x] old_hash is " BRIGHT_WHITE("%lld") " [%#x]\n", now_hash, now_hash, old_hash, old_hash);
        stk->hash = old_hash;
        return STACK_ERRNO::CORRUPT_HASH;
    }
    stk->hash = old_hash;
    return STACK_ERRNO::SUCSSESS;
}

void StackDump_impl(my_stack_t * const stk, STACK_ERRNO stk_errno, const char * const reason,
        const char * file, int line, const char * func) {
    assert(stk      != NULL);
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
    if (stk_errno != STACK_ERRNO::SUCSSESS) {
        printf(BOLD(BRIGHT_RED("STACK IS IN INVALID STATE\n")));
        printf("errno is " RED("%d") " → " BRIGHT_RED("%s") "\n", stk_errno, StackError(stk_errno));
    } else {
            printf(BRIGHT_GREEN("STACK IS VALID\n"));
    }

    printf("reason: " BRIGHT_BLUE("%s\n"), reason);
    printf("stack at " CYAN("%p\n"), stk);

    size_t actual_bytes = 0;
    size_t expected_bytes = stk->capacity * sizeof(stack_element_t);

    if (stk->data != NULL) {
        actual_bytes = malloc_size(stk->data);
    }
    printf("\tleft_canary = %d [%#x] " BRIGHT_GREEN("(CANARY)\n"), stk->canary1, (unsigned) stk->canary1);
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
        printf(BRIGHT_YELLOW("%zu"), stk->size - 1);
    }
    printf(")\n");

    printf("\thash        = " BRIGHT_WHITE("%zu") " [%#zx]\n", stk->hash, stk->hash);
    printf("\tdata_hash   = " BRIGHT_WHITE("%zu") " [%#zx]\n", stk->data_hash, stk->data_hash);

    // Вывод data
    if (stk->data == NULL) {
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
        printf("\t\t" YELLOW(" ") BRIGHT_GREEN("[0]") " = " YELLOW("%d") " [%#x] " BRIGHT_GREEN("(CANARY)\n"), stk->data[0], (unsigned) stk->data[0]);
        for(size_t i = 1; i < total_slots; ++i){
            if (i < stk->size) {
                printf("\t\t" BRIGHT_GREEN("*") BRIGHT_YELLOW("[%zu]") " = " BRIGHT_WHITE("%d") " [%#x]\n", i, stk->data[i], (unsigned) stk->data[i]);
            } else if (i < stk->capacity - 1) {
                printf("\t\t" YELLOW(" ") YELLOW("[%zu]") " = " YELLOW("%d") " [%#x] " BRIGHT_BLACK("(POISON)\n"), i, stk->data[i], (unsigned) stk->data[i]);
            } else if (i == stk->capacity -1) {
                printf("\t\t" YELLOW(" ") BRIGHT_GREEN("[%zu]") " = " YELLOW("%d") " [%#x] " BRIGHT_GREEN("(CANARY)\n"), i, stk->data[i], (unsigned) stk->data[i]);
            } else {
                printf("\t\t" BRIGHT_BLACK(" ") BRIGHT_BLACK("[%zu]") " = " BRIGHT_BLACK("%d") " [%#x] " BRIGHT_BLACK("(padding)\n"), i, stk->data[i], (unsigned) stk->data[i]);
            }
        }
    }

    printf("\t}\n");
    printf("\tright_canary = %d [%#x] " BRIGHT_GREEN("(CANARY)\n"), stk->canary2, (unsigned) stk->canary2);
    printf("}\n");
    printf("================================================\n\n");
}