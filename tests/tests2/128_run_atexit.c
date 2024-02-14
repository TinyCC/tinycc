#include <stdio.h>

int atexit(void (*function)(void));
int on_exit(void (*function)(int, void *), void *arg);
void exit(int status);

void __attribute((constructor)) startup5(void)
{
    printf ("startup5\n");
}

void cleanup1(void)
{
    printf ("cleanup1\n");
    fflush(stdout);
}

void cleanup2(void)
{
    printf ("cleanup2\n");
}

void cleanup3(int ret, void *arg)
{
    printf ("%d %s\n", ret, (char *) arg);
}

void cleanup4(int ret, void *arg)
{
    printf ("%d %s\n", ret, (char *) arg);
}

void __attribute((destructor)) cleanup5(void)
{
    printf ("cleanup5\n");
}

void test(void)
{
    atexit(cleanup1);
    atexit(cleanup2);
    on_exit(cleanup3, "cleanup3");
    on_exit(cleanup4, "cleanup4");
}

#if defined test_128_return
int main(int argc, char **argv)
{
    test();
    return 1;
} 

#elif defined test_128_exit
int main(int argc, char **argv)
{
    test();
    exit(2);
} 
#endif
