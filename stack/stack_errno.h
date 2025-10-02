#ifndef STACK_ERRNO_H
#define STACK_ERRNO_H

#include "policy.h"

enum {
    SUCCESS                          = 0,
    NULL_PTR_PASSED                  = 1,
    CANNOT_ALLOCATE_MEMORY           = 2,
    STACK_EMPTY                      = 3,
    STACK_DATA_IS_NULL_PTR           = 4,
    SIZE_BIGGER_THAN_CAPACITY        = 5,
    STACK_OVERFLOW                   = 6,
    DATABUF_SIZE_NOT_MATCH_CAPACITY  = 7,
    CORRUPT_POISON                   = 8,
    POISON_COLLISION                 = 9,
    WRONG_REALLOC_SIZE               = 10,
    canary_protection(CORRUPT_CANARY = 11,)
    hash_protection(CORRUPT_HASH     = 12,)
    CANNOT_REALLOCATE_MEMORY         = 13,
} typedef STACK_ERRNO;

#endif // STACK_ERRNO_H