#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "stack.h"
#include "ansi.h"

#define begin do
#define end while(0) // Это не цикл. Just блок в котором можно break и continue

#define DO(code) {\
    fprintf(stderr, BRIGHT_CYAN("%s:%d %s() >>> " #code "\n"), __FILE__, __LINE__, __func__);\
    { code; }                           \
    StackDump(stk1, stk_errno, #code);  \
    }

int main() {
    begin {
        STACK_ERRNO stk_errno = STACK_ERRNO::SUCSSESS;
        my_stack_t *stk1 = StackCtor(4, &stk_errno);
        fprintf(stderr, "errno is %d", stk_errno);
        if (stk_errno != STACK_ERRNO::SUCSSESS) break; // Не удалось создать стек -> прекращаем дальнейшее исполнение

        DO(stk_errno = StackPush(stk1, 10));
        DO(stk_errno = StackPush(stk1, 20));
        DO(stk_errno = StackPush(stk1, 30));

        stack_element_t value;

        DO(stk_errno = StackPop(stk1, &value));
        printf("%d\n", value);

        DO(stk_errno = StackPush(stk1, 40));

        DO(stk_errno = StackPush(stk1, (int) 0x96716f66));
        // POISON COLLISION - ERROR
        DO(stk_errno = StackPush(stk1, 50));
        do {
            DO(stk_errno = StackPush(stk1, INT_MAX));
            if (stk_errno == SUCSSESS)
                break;
            else
                DO(StackRealloc(stk1, 15));
        } while (1);

        DO(StackPop(stk1, &value));
        DO(StackRealloc(stk1, 10));

        for(;;){
            if ((stk_errno = StackPop(stk1, &value)) == STACK_ERRNO::SUCSSESS) {
                StackDump(stk1, stk_errno, "for (;;) StackPop(...);");
            } else
                break;
        }

        StackDtor(stk1);
    } end;

    return 0;
}