#include <stdio.h>

/* Test 1: Global scope - should pass */
_Static_assert(1, "basic test");
_Static_assert(sizeof(int) >= 2, "int size");
_Static_assert(sizeof(char) == 1, "char size");

/* Test 2: Complex expressions */
_Static_assert(sizeof(char) < sizeof(int), "size comparison");
_Static_assert((1 << 3) == 8, "bit operations");

/* Test 3: String concatenation */
_Static_assert(1, "multi" "ple " "strings");

/* Test 4: In struct */
struct test {
    int x;
    _Static_assert(sizeof(int) == 4, "struct scope");
};

/* Test 5: Macro with _Static_assert */
#define CHECK_SIZE(type, max_size) \
    _Static_assert(sizeof(type) <= max_size, "type too large")

CHECK_SIZE(int, 8);

int main() {
    /* Test 6: Function scope */
    _Static_assert(sizeof(void*) >= sizeof(int), "pointer size");

    /* Test 7: In statement context (like in a macro) */
    do {
        _Static_assert(sizeof(char) == 1, "char is 1 byte");
        printf("Test 7: Statement context assertion passed\n");
    } while (0);

    printf("All static assertions passed!\n");
    return 0;
}
