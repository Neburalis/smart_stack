#ifndef STACK_HANDLERS_H
#define STACK_HANDLERS_H

#include <stdlib.h>
#include "stack_errno.h"

int typedef stack_element_t;

typedef size_t StackHandler;
#define STACK_HANDLERS_ARRAY_SIZE 128

#define StackDumpH(stk, stk_errno, reason) \
    StackDumpH_impl(stk, stk_errno, reason, __FILE__, __LINE__, __PRETTY_FUNCTION__)

StackHandler StackCtorH(const size_t capacity, STACK_ERRNO * stk_errno);

STACK_ERRNO StackPushH(StackHandler handler, stack_element_t value);

STACK_ERRNO StackPopH(StackHandler handler, stack_element_t * value);

STACK_ERRNO StackReallocH(StackHandler handler, size_t new_size);

STACK_ERRNO StackDtorH(StackHandler handler);

const char * StackErrorH(STACK_ERRNO stk_errno);

void StackDumpH_impl(StackHandler handler, STACK_ERRNO stk_errno, const char * const reason,
    const char * file, int line, const char * func);

#endif // STACK_HANDLERS_H