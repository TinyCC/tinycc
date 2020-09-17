#include <stdio.h>
#include <stdarg.h>

void tst3(void)
{
  /* Should VT_SYM be checked for TOK_builtin_constant_p */
  int r = __builtin_constant_p("c");
  if (r == 0) printf("%d\n",r);
}

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
#if TEST == 4
  {
    /* Only integer allowed */
     __builtin_return_address(0 + 1) != NULL;
  }
#endif
  return 0;
}

int
main(void)
{
  tst3();
}
