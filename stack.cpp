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

struct my_stack{
    int                 canary1;
    size_t              size;
    size_t              capacity;
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
// } typedef STACK_ERRNO;

#define POISON  0x96716f66
#define CANARY1 0xDED0AFFF
#define CANARY2 0xEDA0FAAA
#define CANARY3 0xDED00EDA
#define CANARY4 0xABCD1234

#define StackValidate()                                             \
    begin {                                                         \
        STACK_ERRNO stk_errno = StackValidator(stk);                \
        if (stk_errno != STACK_ERRNO::SUCSSESS) {                   \
            StackDump(stk, stk_errno, "stk_errno is not sucssess"); \
            return stk_errno;                                       \
        }                                                           \
    } end;

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
    my_stack_t * stk = (my_stack_t *) calloc(1, sizeof(my_stack_t));
    if (stk == NULL) {
        *stk_errno = STACK_ERRNO::CANNOT_ALLOCATE_MEMORY;
        return NULL;
    }

    stk->canary1 = CANARY1;
    stk->canary2 = CANARY2;

    capacity = cpl2(capacity);
    stk->capacity = capacity;

    stk->data = (stack_element_t *) calloc(capacity, sizeof(stack_element_t));
    if (stk->data == NULL) {
        *stk_errno = STACK_ERRNO::CANNOT_ALLOCATE_MEMORY;
        return NULL;
    }
    stk->size = 0;

    for (size_t i = 0; i < malloc_size(stk->data) / sizeof(stack_element_t); ++i) {
        // calloc может выделить больше памяти чем попросили. Заполняем poison'ом ВСЕ
        stk->data[i] = POISON;
    }

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

    StackValidate();
    return STACK_ERRNO::SUCSSESS;
}

STACK_ERRNO StackPop(my_stack_t * const stk, stack_element_t * value) {
    StackValidate();

    if (stk->size == 0) return STACK_ERRNO::STACK_EMPTY;
    *value = stk->data[--stk->size];

    stk->data[stk->size] = POISON;

    StackValidate();
    return STACK_ERRNO::SUCSSESS;
}

STACK_ERRNO StackDtor(my_stack_t * stk) {
    StackValidate();

    free(stk->data);
    stk->data = NULL;

    free(stk);
    stk = NULL;

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
        default:                                            return "Unknown stack error";
    }
}

STACK_ERRNO StackValidator(my_stack_t * const stk) {
    if (stk == NULL) // Стек - nullptr
        return STACK_ERRNO::NULL_PTR_PASSED;
    if (stk->canary1 != CANARY1 || stk->canary2 != CANARY2)
        return STACK_ERRNO::CORRUPT_CANARY;
    if (stk->data == NULL) // Динамический массив - не создан
        return STACK_ERRNO::STACK_DATA_IS_NULL_PTR;
    if (stk->size > stk->capacity) // Количество элементов в стеке больше чем его вместимость
        return STACK_ERRNO::SIZE_BIGGER_THAN_CAPACITY;
    size_t total_slots = malloc_size(stk->data) / sizeof(stack_element_t);
    if (total_slots < stk->capacity) // Размер динамической памяти меньше вместимости
        return STACK_ERRNO::DATABUF_SIZE_NOT_MATCH_CAPACITY;
    for (size_t i = stk->size; i < total_slots; ++i) {
        if (stk->data[i] != POISON)
            return STACK_ERRNO::CORRUPT_POISON;
    }
    return STACK_ERRNO::SUCSSESS;
}

void StackDump_impl(my_stack_t * const stk, STACK_ERRNO stk_errno, const char * const reason,
        const char *file, int line, const char *func) {
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

    // Вывод capacity
    if (stk->data != NULL && actual_bytes != expected_bytes) {
        printf("\tcapacity  = " BRIGHT_WHITE("%zu") " (expected " CYAN("%zu bytes") ")\n", stk->capacity, expected_bytes);
    } else {
        printf("\tcapacity  = " BRIGHT_WHITE("%zu") "\n", stk->capacity);
    }

    printf("\tsize      = " BRIGHT_WHITE("%zu") " (last element index = ", stk->size);
    if (stk->size == 0) {
        printf(BRIGHT_RED("NONE"));
    } else {
        printf(BRIGHT_YELLOW("%zu"), stk->size - 1);
    }
    printf(")\n");

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
        for(size_t i = 0; i < total_slots; ++i){
            if (i < stk->size) {
                printf("\t\t" BRIGHT_GREEN("*") BRIGHT_YELLOW("[%zu]") " = " BRIGHT_WHITE("%d") " [%#X]\n", i, stk->data[i], stk->data[i]);
            } else if (i < stk->capacity) {
                printf("\t\t" YELLOW(" ") YELLOW("[%zu]") " = " YELLOW("%d") " [%#X] " BRIGHT_BLACK("(POISON)\n"), i, stk->data[i], stk->data[i]);
            } else {
                printf("\t\t" BRIGHT_BLACK(" ") BRIGHT_BLACK("[%zu]") " = " BRIGHT_BLACK("%d") " [%#X] " BRIGHT_BLACK("(padding)\n"), i, stk->data[i], stk->data[i]);
            }
        }
    }

    printf("\t}\n}\n");
    printf("================================================\n\n");
}