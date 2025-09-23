#ifndef STACK_H
#define STACK_H

int typedef stack_element_t;

struct {
    size_t size;
    size_t capacity;
    stack_element_t * data;
} typedef my_stack_t;

enum {
    SUCSSESS = 0,
    NULL_PTR_PASSED,
    CANNOT_ALLOCATE_MEMORY,
    STACK_EMPTY,
} typedef STACK_ERRNO;

// Нахождение ближайшей степени 2 >= x
size_t cpl2(size_t x);

STACK_ERRNO StackCtor(my_stack_t * stk, size_t capacity);

STACK_ERRNO StackPush(my_stack_t * stk, stack_element_t value);

STACK_ERRNO StackPop(my_stack_t * stk, stack_element_t * value);

STACK_ERRNO StackDtor(my_stack_t * stk);

void StackDump(my_stack_t * const stk, STACK_ERRNO errno, const char * const reason);

#endif // STACK_H