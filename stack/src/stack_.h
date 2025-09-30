#ifndef STACK_INTERNAL_H
#define STACK_INTERNAL_H

#include "policy.h"
#include "stack_errno.h"

int typedef stack_element_t;

struct my_stack {
    T1(int              canary1;)

    size_t              size;
    size_t              capacity;
    stack_element_t *   data;

    T3(size_t           hash;)
    T3(size_t           data_hash;)

    T1(int              canary2;)
} typedef my_stack_t;

#define StackDumpI(stk, stk_errno, reason) \
    StackDumpI_impl(stk, stk_errno, reason, __FILE__, __LINE__, __PRETTY_FUNCTION__)

my_stack_t * StackCtorI(size_t capacity, STACK_ERRNO * stk_errno);

STACK_ERRNO StackPushI(my_stack_t * stk, stack_element_t value);

STACK_ERRNO StackPopI(my_stack_t * stk, stack_element_t * value);

STACK_ERRNO StackReallocI(my_stack_t * const stk, size_t new_size);

STACK_ERRNO StackDtorI(my_stack_t * stk);

STACK_ERRNO StackValidatorI(my_stack_t * const stk);

const char * StackErrorI(STACK_ERRNO stk_errno);

void StackDumpI_impl(my_stack_t * const stk, STACK_ERRNO stk_errno, const char * const reason,
    const char *file, int line, const char *func);

#endif // STACK_INTERNAL_H