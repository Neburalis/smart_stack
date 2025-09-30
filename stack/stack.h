#ifndef STACK_H
#define STACK_H

#include "policy.h"
#include "stack_errno.h"

#if 0 //tolerance_lvl >= 1
    // Работа с хендлерами, пользователь не имеет указателя на структуру

#else
    // Работа напрямую с указателем на структуру

    #include "src/stack_.h"

    typedef struct my_stack my_stack_t;

    #define StackDump(stk, stk_errno, reason)           StackDumpI(stk, stk_errno, reason)
    #define StackCtor(capacity, stk_errno)              StackCtorI(capacity, stk_errno)
    #define StackPush(stk, value)                       StackPushI(stk, value)
    #define StackPop(stk, value)                        StackPopI(stk, value)
    #define StackRealloc(stk, new_size)                 StackReallocI(stk, new_size)
    #define StackDtor(stk)                              StackDtorI(stk)
    #define StackValidator(stk)                         StackValidatorI(stk)
    #define StackError(stk_errno)                       StackErrorI(stk_errno)
#endif

#endif // STACK_H