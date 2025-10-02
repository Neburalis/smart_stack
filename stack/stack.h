#ifndef STACK_H
#define STACK_H

#include "policy.h"
#include "stack_errno.h"

#ifdef enable_handlers_protection
    // Работа с хендлерами, пользователь не имеет указателя на структуру
    #include "handlers/handlers.h"

    #define StackDump(stk, stk_errno, reason)           StackDumpH(stk, stk_errno, reason)
    #define StackCtor(capacity, stk_errno)              StackCtorH(capacity, stk_errno)
    #define StackPush(stk, value)                       StackPushH(stk, value)
    #define StackPop(stk, value)                        StackPopH(stk, value)
    #define StackRealloc(stk, new_size)                 StackReallocH(stk, new_size)
    #define StackDtor(stk)                              StackDtorH(stk)
    #define StackError(stk_errno)                       StackErrorH(stk_errno)
#else
    // Работа напрямую с указателем на структуру
    #include "src/stack_.h"

    typedef my_stack * StackHandler;

    #define StackDump(stk, stk_errno, reason)           StackDumpI(stk, stk_errno, reason)
    #define StackCtor(capacity, stk_errno)              StackCtorI(capacity, stk_errno)
    #define StackPush(stk, value)                       StackPushI(stk, value)
    #define StackPop(stk, value)                        StackPopI(stk, value)
    #define StackRealloc(stk, new_size)                 StackReallocI(stk, new_size)
    #define StackDtor(stk)                              StackDtorI(stk)
    #define StackError(stk_errno)                       StackErrorI(stk_errno)
#endif

#endif // STACK_H