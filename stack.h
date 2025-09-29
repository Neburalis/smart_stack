#ifndef STACK_H
#define STACK_H

#include "policy.h"

int typedef stack_element_t;

typedef struct my_stack my_stack_t;

enum {
    SUCSSESS                        = 0,
    NULL_PTR_PASSED                 = 1,
    CANNOT_ALLOCATE_MEMORY          = 2,
    STACK_EMPTY                     = 3,
    STACK_DATA_IS_NULL_PTR          = 4,
    SIZE_BIGGER_THAN_CAPACITY       = 5,
    STACK_OVERFLOW                  = 6,
    DATABUF_SIZE_NOT_MATCH_CAPACITY = 7,
    CORRUPT_POISON                  = 8,
    POISON_COLLISION                = 9,
    WRONG_REALLOC_SIZE              = 10,
    T1(CORRUPT_CANARY               = 11,)
    T3(CORRUPT_HASH                 = 12,)
    CANNOT_REALLOCATE_MEMORY        = 13,
} typedef STACK_ERRNO;

#define StackDump(stk, stk_errno, reason) \
    StackDump_impl(stk, stk_errno, reason, __FILE__, __LINE__, __PRETTY_FUNCTION__)

my_stack_t * StackCtor(size_t capacity, STACK_ERRNO * stk_errno);

STACK_ERRNO StackPush(my_stack_t * stk, stack_element_t value);

STACK_ERRNO StackPop(my_stack_t * stk, stack_element_t * value);

STACK_ERRNO StackRealloc(my_stack_t * const stk, size_t new_size);

STACK_ERRNO StackDtor(my_stack_t * stk);

const char * StackError(STACK_ERRNO stk_errno);

STACK_ERRNO StackValidator(my_stack_t * const stk);

void StackDump_impl(my_stack_t * const stk, STACK_ERRNO stk_errno, const char * const reason,
    const char *file, int line, const char *func);

#endif // STACK_H