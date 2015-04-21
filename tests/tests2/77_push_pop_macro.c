#include <stdio.h>

int main()
{
    #define abort "111"
    printf("abort = %s\n", abort);

    #pragma push_macro("abort")
    #undef abort
    #define abort "222"
    printf("abort = %s\n", abort);

    #pragma push_macro("abort")
    #undef abort
    #define abort "333"
    printf("abort = %s\n", abort);

    #pragma pop_macro("abort")
    printf("abort = %s\n", abort);

    #pragma pop_macro("abort")
    printf("abort = %s\n", abort);
}
