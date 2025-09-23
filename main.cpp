#include <stdio.h>

#include "stack.h"

int main() {
    my_stack_t stk1 = {};
    StackCtor(&stk1, 5);

    StackPush(&stk1, 10);
    StackDump(&stk1, STACK_ERRNO::SUCSSESS, "ЛФИ - позор россии");
    StackPush(&stk1, 20);
    StackDump(&stk1, STACK_ERRNO::SUCSSESS, "ЛФИ - позор россии");
    StackPush(&stk1, 30);
    StackDump(&stk1, STACK_ERRNO::SUCSSESS, "ЛФИ - позор россии");

    stack_element_t value;

    for(;;){
        if (StackPop(&stk1, &value) == STACK_ERRNO::SUCSSESS)
            printf("%d\n", value);
        else
            break;
    }

    return 0;
}