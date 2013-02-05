#include <stdio.h>

enum fred
{
   a,
   b,
   c,
   d,
   e = 54,
   f = 73,
   g,
   h
};

int main()
{
   enum fred frod;

   printf("%d %d %d %d %d %d %d %d\n", a, b, c, d, e, f, g, h);
   /* printf("%d\n", frod); */
   frod = 12;
   printf("%d\n", frod);
   frod = e;
   printf("%d\n", frod);

   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/
