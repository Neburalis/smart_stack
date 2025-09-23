#include <assert.h>
#include <stdio.h>
#include <malloc/malloc.h>
#include "stack.h"

// int typedef stack_element_t;
//
// struct {
//     size_t size;
//     size_t capacity;
//     stack_element_t * data;
// } typedef my_stack_t;
//
// enum {
//     SUCSSESS = 0,
//     NULL_PTR_PASSED,
//     CANNOT_ALLOCATE_MEMORY,
//     STACK_EMPTY,
// } typedef STACK_ERRNO;

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

STACK_ERRNO StackCtor(my_stack_t * const stk, size_t capacity) {
    if(stk == NULL) return STACK_ERRNO::NULL_PTR_PASSED;

    capacity = cpl2(capacity);

    stk->data = (stack_element_t *) calloc(capacity, sizeof(stack_element_t));
    if (stk->data == NULL)
        return STACK_ERRNO::CANNOT_ALLOCATE_MEMORY;
    stk->size = 0;

    return STACK_ERRNO::SUCSSESS;
}

STACK_ERRNO StackPush(my_stack_t * const stk, stack_element_t value) {
    if(stk == NULL) return STACK_ERRNO::NULL_PTR_PASSED;

    stk->data[stk->size++] = value;

    return STACK_ERRNO::SUCSSESS;
}

STACK_ERRNO StackPop(my_stack_t * const stk, stack_element_t * value) {
    if(stk == NULL) return STACK_ERRNO::NULL_PTR_PASSED;

    if (stk->size == 0) return STACK_ERRNO::STACK_EMPTY;
    *value = stk->data[--stk->size];

    return STACK_ERRNO::SUCSSESS;
}

STACK_ERRNO StackDtor(my_stack_t * const stk) {
    if(stk == NULL) return STACK_ERRNO::NULL_PTR_PASSED;

    free(stk->data);
    stk->data = NULL;

    return STACK_ERRNO::SUCSSESS;
}

void StackDump(my_stack_t * const stk, STACK_ERRNO errno, const char * const reason) {
    assert(stk != NULL);

    printf("stack at %p\n", stk);
    printf("reason is %s\n", reason);
    printf("{\n"
           "\tsize = %zu\n"
           "\tcapacity = %zu\n"
           "\tdata[%zu] at %p\n\t{\n", stk->size, stk->capacity, malloc_size(stk->data), stk->data);
    for(size_t i = 0; i < malloc_size(stk->data)/sizeof(stack_element_t); ++i){
        printf("\t\t%c[%zu] = %d\n", i<stk->size?'*':' ', i, stk->data[i]);
    }
    printf("\t}\n}\n");
}