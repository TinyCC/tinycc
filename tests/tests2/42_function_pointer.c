#include <stdio.h>

int fred(int p)
{
   printf("yo %d\n", p);
   return 42;
}

int (*f)(int) = &fred;

/* To test what this is supposed to test the destination function
   (fprint here) must not be called directly anywhere in the test.  */
int (*fprintfptr)(FILE *, const char *, ...) = &fprintf;

typedef int (*func) (int);
static int dummy1(int i) { return 0; }
int dummy2(int i) { return 0; }
static func allfunc[] = { putchar, dummy1, dummy2 };

int main()
{
   fprintfptr(stdout, "%d\n", (*f)(24));

   printf ("%d\n", allfunc[0] == putchar);
   printf ("%d\n", allfunc[1] == dummy1);
   printf ("%d\n", allfunc[2] == dummy2);
   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/
