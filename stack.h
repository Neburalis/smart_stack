#ifndef STACK_H
#define STACK_H

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
    CORRUPT_CANARY                  = 10,
    CORRUPT_HASH                    = 11,
} typedef STACK_ERRNO;

#define StackDump(stk, stk_errno, reason) \
    StackDump_impl(stk, stk_errno, reason, __FILE__, __LINE__, __PRETTY_FUNCTION__)

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
unsigned long long sdbm(char * data, size_t max_len);

// Нахождение ближайшей степени 2 >= x
size_t cpl2(size_t x);

my_stack_t * StackCtor(size_t capacity, STACK_ERRNO * stk_errno);

STACK_ERRNO StackPush(my_stack_t * stk, stack_element_t value);

STACK_ERRNO StackPop(my_stack_t * stk, stack_element_t * value);

STACK_ERRNO StackDtor(my_stack_t * stk);

const char * StackError(STACK_ERRNO stk_errno);

STACK_ERRNO StackValidator(my_stack_t * const stk);

void StackDump_impl(my_stack_t * const stk, STACK_ERRNO stk_errno, const char * const reason,
    const char *file, int line, const char *func);

#endif // STACK_H