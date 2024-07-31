/* This test only works on x86-64. */
/* Previously in TinyCC, ELF sections defined in assembly would always have the
execute bit not set, so you would get segmentation faults when code in these
sections was exectuted. This file is a minimal example of a file that will put
the resulting code in a non-executable section (and invoke it) prior to the fix.
*/
#include <stdio.h>

void *memset(void *dst, int c, int len);

__asm__ (
".section .text.nolibc_memset\n"
".weak memset\n"
"memset:\n"
	"xchgl %eax, %esi\n\t"
	"movq  %rdx, %rcx\n\t"
	"pushq %rdi\n\t"
	"rep stosb\n\t"
	"popq  %rax\n\t"
	"retq\n"
);

int main () {
    char buf[10];
    memset(&buf[0], 'A', 9);
    buf[9] = 0;
    puts(buf);
    return 0;
}
