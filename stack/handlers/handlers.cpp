#include "stack_.h"
#include "handlers.h"

static my_stack * Handlers_arr[STACK_HANDLERS_SIZE] = {};
static StackHandler FreeHandler = 0;

static StackHandler addToHandlers(my_stack * ptr) {
    
}