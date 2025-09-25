#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "stack.h"

#define begin do
#define end while(0) // Это не цикл. Just блок в котором можно break и continue

int main() {
    begin {
        STACK_ERRNO stk_errno = STACK_ERRNO::SUCSSESS;
        my_stack_t *stk1 = StackCtor(4, &stk_errno);
        if (stk_errno != STACK_ERRNO::SUCSSESS) break; // Не удалось создать стек -> прекращаем дальнейшее исполнение

        stk_errno = StackPush(stk1, 10);
        StackDump(stk1, stk_errno, "just dump from main");
        stk_errno = StackPush(stk1, 20);
        StackDump(stk1, stk_errno, "just dump from main");
        stk_errno = StackPush(stk1, 30);
        StackDump(stk1, stk_errno, "just dump from main");

        stack_element_t value;

        StackPop(stk1, &value);
        printf("%d\n", value);

        stk_errno = StackPush(stk1, 40);
        StackDump(stk1, stk_errno, "just dump from main");
        stk_errno = StackPush(stk1, 0x96716f66);
        StackDump(stk1, stk_errno, "just dump from main");
        // POISON COLLISION - ERROR
        stk_errno = StackPush(stk1, 50);
        StackDump(stk1, stk_errno, "just dump from main");
        stk_errno = StackPush(stk1, INT_MAX);
        StackDump(stk1, stk_errno, "just dump from main");

        for(;;){
            if ((stk_errno = StackPop(stk1, &value)) == STACK_ERRNO::SUCSSESS) {
                StackDump(stk1, stk_errno, "");
                printf("%d\n", value);
            } else
                break;
        }

        StackDtor(stk1);
    } end;

    return 0;
}