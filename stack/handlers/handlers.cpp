#include <assert.h>
#include <stdio.h>
#include "stack_.h"
#include "handlers.h"
#include "io_utils.h"

static my_stack * HandlersArr[STACK_HANDLERS_ARRAY_SIZE] = {};
static StackHandler NextHandler = 0;
static size_t       HandlersCount = 0;

static StackHandler addToHandlersArr(my_stack * ptr) {
    assert(ptr);
    assert(HandlersCount < STACK_HANDLERS_ARRAY_SIZE);
    assert(HandlersArr[NextHandler] == NULL);

    StackHandler handler = NextHandler;
    while (HandlersArr[++NextHandler]); // Двигаем на ближайший не NULL

    HandlersArr[handler] = ptr;

    return handler;
}

static my_stack * popFromHandler(StackHandler handler) {
    assert(HandlersCount < STACK_HANDLERS_ARRAY_SIZE);
    assert(HandlersArr[NextHandler] == NULL);

    if (HandlersArr[handler] != NULL) {
        my_stack * stk = HandlersArr[handler];
        HandlersArr[handler] = stk;
        if (NextHandler > handler) NextHandler = handler;
        return stk;
    } else return NULL;
}

StackHandler StackCtorH(const size_t capacity, STACK_ERRNO * stk_errno) {
    assert(HandlersCount < STACK_HANDLERS_ARRAY_SIZE);
    assert(HandlersArr[NextHandler] == NULL);

    my_stack * ptr = StackCtorI(capacity, stk_errno);
    return addToHandlersArr(ptr);
}

STACK_ERRNO StackPushH(StackHandler handler, stack_element_t value) {
    assert(HandlersCount < STACK_HANDLERS_ARRAY_SIZE);
    assert(HandlersArr[NextHandler] == NULL);

    printf("handler in %s is %zu (%p) [%p]\n", __PRETTY_FUNCTION__, handler, handler, HandlersArr[handler]);

    STACK_ERRNO stk_errno = StackPushI(HandlersArr[handler], value);
    return stk_errno;
}

STACK_ERRNO StackPopH(StackHandler handler, stack_element_t * value) {
    assert(HandlersCount < STACK_HANDLERS_ARRAY_SIZE);
    assert(HandlersArr[NextHandler] == NULL);

    STACK_ERRNO stk_errno = StackPopI(HandlersArr[handler], value);
    return stk_errno;
}

STACK_ERRNO StackReallocH(StackHandler handler, size_t new_size) {
    assert(HandlersCount < STACK_HANDLERS_ARRAY_SIZE);
    assert(HandlersArr[NextHandler] == NULL);

    STACK_ERRNO stk_errno = StackReallocI(HandlersArr[handler], new_size);
    return stk_errno;
}

STACK_ERRNO StackDtorH(StackHandler handler) {
    assert(HandlersCount < STACK_HANDLERS_ARRAY_SIZE);
    assert(HandlersArr[NextHandler] == NULL);

    STACK_ERRNO stk_errno = StackDtorI(HandlersArr[handler]);
    if (stk_errno != STACK_ERRNO::SUCCESS)
        return stk_errno;
    popFromHandler(handler);
    return STACK_ERRNO::SUCCESS;
}

const char * StackErrorH(STACK_ERRNO stk_errno) {
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
        T1(case STACK_ERRNO::CORRUPT_CANARY:                return "Canary is spoiled => stack is damaged";)
        T3(case STACK_ERRNO::CORRUPT_HASH:                  return "Hash is spoiled => stack is damaged";)
        case STACK_ERRNO::CANNOT_REALLOCATE_MEMORY:         return "Realloc returned NULL PTR, this error is not fatal, stack data was not deleted or freed";
        default:                                            return "Unknown stack error";
    }
}

void StackDumpH_impl(StackHandler const handler, STACK_ERRNO stk_errno, const char * const reason,
const char * file, int line, const char * func) {
    assert(HandlersCount < STACK_HANDLERS_ARRAY_SIZE);
    assert(HandlersArr[NextHandler] == NULL);

    assert(reason   != NULL);
    assert(file     != NULL);
    assert(func     != NULL);

    printf("================== " BRIGHT_WHITE("HANDLED STACK DUMP") " ==================\n");
    printf("handler is %zu\n", handler);

    StackDumpI_impl(HandlersArr[handler], stk_errno, reason,
        file, line, func);
}