#include <stdio.h>
#include <assert.h>

char arr[1];
static void f (void){}
void (*fp)(void) = f;
void call_fp()
{
    (fp?f:f)();
    (fp?fp:fp)();
    (fp?fp:&f)();
    (fp?&f:fp)();
    (fp?&f:&f)();
    _Generic(0?arr:arr, char*: (void)0);
    _Generic(0?&arr[0]:arr, char*: (void)0);
    _Generic(0?arr:&arr[0], char*: (void)0);
    _Generic(1?arr:arr, char*: (void)0);
    _Generic(1?&arr[0]:arr, char*: (void)0);
    _Generic(1?arr:&arr[0], char*: (void)0);
    _Generic((__typeof(1?f:f)*){0}, void (**)(void): (void)0);
    (fp?&f:f)();
    (fp?f:&f)();
    _Generic((__typeof(fp?0L:(void)0)*){0}, void*: (void)0);

    //Should cleanly fail, not segfault:
    /*(fp?f:1);*/
}

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
