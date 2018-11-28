#include <stdio.h>
#include <assert.h>

int main()
{
   int Count;

   for (Count = 0; Count < 10; Count++)
   {
      printf("%d\n", (Count < 5) ? (Count*Count) : (Count * 3));
   }

   {
    int c = 0;
    #define ASSERT(X) assert(X)
    static struct stru { int x; } a={'A'},b={'B'};
    ASSERT('A'==(*(1?&a:&b)).x);
    ASSERT('A'==(1?a:b).x);
    ASSERT('A'==(c?b:a).x);
    ASSERT('A'==(0?b:a).x);
    c=1;
    ASSERT('A'==(c?a:b).x);
   }


   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/
