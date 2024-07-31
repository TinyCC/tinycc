#include <stdio.h>

/* Previously in TinyCC, ELF sections defined in attributes would always have
the execute bit not set, so you would get segmentation faults when code in these
sections was exectuted. This file is a minimal example of a file that will put
the resulting code in a non-executable section (and invoke it) prior to the fix.
*/
__attribute__((section(".text.wumbo")))
int wumbo (int arg) {
    return arg * 2;
}

int main () {
    wumbo(2);
    puts("hi");
    return 0;
}
