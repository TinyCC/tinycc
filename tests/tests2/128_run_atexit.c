#include <stdio.h>
#include <stdlib.h>

void cleanup1(void)
{
    printf ("cleanup1\n");
}

void cleanup2(void)
{
    printf ("cleanup2\n");
}

int main(int argc, char **argv)
{
    atexit(cleanup1);
    atexit(cleanup2);
    return 0;
} 

