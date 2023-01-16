#include <stdio.h>
#include <stdarg.h>

int compile_errors(void)
{
#if TEST == 1
  {
    /* Not constant */
    static int i = (&"Foobar"[1] - &"Foobar"[0]);
  }
#endif
#if TEST == 2
  {
    /* Not constant */
    struct{int c;}v;
    static long i=((char*)&(v.c)-(char*)&v);
  }
#endif
#if TEST == 3
  {
    /* Not constant */
    static const short ar[] = { &&l1 - &&l1, &&l2 - &&l1 };
    void *p = &&l1 + ar[0];
    goto *p;
   l1: return 1;
   l2: return 2;
  }
#endif
  return 0;
}

int
main(void)
{
}
