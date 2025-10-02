#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "stack_errno.h"
#include "stack.h"
#include "ansi.h"

#define begin do
#define end while(0) // Это не цикл. Just блок в котором можно break и continue

// TODO - условная компиляция
// TODO - раздельное включение защит
// TODO калькулятор
// TODO возвращать индекс в массиве (хэндлеры)

#define DO(code) {\
    fprintf(stderr, BRIGHT_CYAN("%s:%d %s() >>> " #code "\n"), __FILE__, __LINE__, __func__);\
    { code; }                           \
    StackDump(stk1, stk_errno, #code);  \
    }

int main() {
    begin {
        STACK_ERRNO stk_errno = STACK_ERRNO::SUCCESS;
        StackHandler stk1 = StackCtor(4, &stk_errno);
        fprintf(stderr, "errno of stk1 is %d\n", stk_errno);
        StackHandler stk2 = StackCtor(5, &stk_errno);
        fprintf(stderr, "errno of stk2 is %d\n", stk_errno);
        if (stk_errno != STACK_ERRNO::SUCCESS) break; // Не удалось создать стек -> прекращаем дальнейшее исполнение

        printf("stk1 is %zu (%p), stk2 is %zu (%p)\n", stk1, stk1, stk2, stk2);

        DO(stk_errno = StackPush(stk1, 10));
        DO(stk_errno = StackPush(stk1, 20));
        DO(stk_errno = StackPush(stk1, 30));

        stk_errno = StackPush(stk2, 10);
        StackDump(stk2, stk_errno, "main:39");
        stk_errno = StackPush(stk2, 20);
        StackDump(stk2, stk_errno, "main:41");
        stk_errno = StackPush(stk2, 30);
        StackDump(stk2, stk_errno, "main:43");

        stack_element_t value;

        DO(stk_errno = StackPop(stk1, &value));
        printf("%d\n", value);

        DO(stk_errno = StackPush(stk1, 40));

        DO(stk_errno = StackPush(stk1, (int) 0x96716f66));
        // POISON COLLISION - ERROR
        DO(stk_errno = StackPush(stk1, 50));
        do {
            DO(stk_errno = StackPush(stk1, INT_MAX));
            if (stk_errno == SUCCESS)
                break;
            else
                DO(StackRealloc(stk1, 15));
        } while (1);

        DO(StackPop(stk1, &value));
        DO(StackRealloc(stk1, 10));

        for(;;){
            if ((stk_errno = StackPop(stk1, &value)) == STACK_ERRNO::SUCCESS) {
                StackDump(stk1, stk_errno, "for (;;) StackPop(...);");
            } else
                break;
        }

        StackDtor(stk1);
    } end;

    return 0;
}