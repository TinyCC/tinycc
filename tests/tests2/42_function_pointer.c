#include <stdio.h>

int fred(int p)
{
   printf("yo %d\n", p);
   return 42;
}

int (*f)(int) = &fred;

int main()
{
   printf("%d\n", (*f)(24));

   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/
