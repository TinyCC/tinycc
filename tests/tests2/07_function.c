#include <stdio.h>

int myfunc(int x)
{
   return x * x;
}

void vfunc(int a)
{
   printf("a=%d\n", a);
}

void qfunc()
{
   printf("qfunc()\n");
}

#if !defined(__ARMEL__)
/*
 * At least on ARM (like RPi), zfunc below fails with something like:
 * +tcc: error: can't relocate value at 1ef93bc,1
 * Test is temporary removed for this architecture until ARM maintainers
 * see what happens with this test.
 */
void zfunc()
{
	((void (*)(void))0) ();
}
#endif

int main()
{
   printf("%d\n", myfunc(3));
   printf("%d\n", myfunc(4));

   vfunc(1234);

   qfunc();

   return 0;
}

// vim: set expandtab ts=4 sw=3 sts=3 tw=80 :
