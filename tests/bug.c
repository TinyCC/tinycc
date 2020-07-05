#include <stdio.h>
#include <stdarg.h>

void tst1(void)
{
  /* problem in gen_cast. Should mask unsigned types */
  signed char c = (signed char) 0xffffffff;
  int r = (unsigned short) c ^ (signed char) 0x99999999;
  if (r != 0xffff0066) printf ("%x\n", r);
}

typedef struct{double x,y;}p;

void tst2(int n,...)
{
  /* va_arg for struct double does not work on some targets */
  int i;
  va_list args;
  va_start(args,n);
  for (i = 0; i < n; i++) {
    p v = va_arg(args,p);
    if (v.x != 1 || v.y != 2) printf("%g %g\n", v.x, v.y);
  }
  va_end(args);
}

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
  p v = { 1, 2};
  tst1();
  tst2(1, v);
  tst3();
}
