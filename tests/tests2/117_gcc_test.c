#include <stdio.h>

void tst_branch(void)
{
  goto *&&a; 
  printf ("dummy");
a: ;
}

void tst_void_ptr(void *pv, int i) 
{
  i ? *pv : *pv; // dr106
}

void tst_shift(void)
{
  int i = 1;
  long l = 1;

  i = i << 32; // illegal. just test
  l = l << 64; // illegal. just test
}

#if !defined(_WIN32)
#include <sys/mman.h>

void tst_const_addr(void)
{
  void *addr = mmap ((void *)0x20000000, 4096, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS, -1, 0);
  if (addr != (void *) -1) {
#if !defined(__riscv)
    *(int *)0x20000000 += 42;
#endif
    munmap (addr, 4096);
  }
}
#endif

struct zero_struct {};

struct zero_struct tst_zero_struct(void)
{
  struct zero_struct ret;

  return ret;
}

struct big_struct { char a[262144]; };

struct big_struct tst_big(struct big_struct tst)
{
   return tst;
}

void tst_adr (int (*fp)(char *, const char *, ...))
{
  char buf[10];

  (*fp)(buf, "%.0f", 5.0);
}

static const char str[] = "abcdefghijklmnopqrstuvwxyz";

void tst_builtin(void)
{
  char *p;
  char tmp[100];

  if (__builtin_offsetof(struct big_struct, a) != 0) __builtin_abort();

  p = __builtin_memcpy (tmp, str, sizeof(str));
  if (p != tmp) __builtin_abort();

  if (__builtin_memcmp (p, str, sizeof(str))) __builtin_abort();

  p = __builtin_memmove(tmp, str, sizeof(str));
  if (__builtin_memcmp (p, str, sizeof(str))) __builtin_abort();

  p = __builtin_memset(tmp, 0, sizeof (tmp));
  if (p != tmp || tmp[0] != 0 || tmp[99] != 0) __builtin_abort();

  if (__builtin_strlen(str) != sizeof(str) - 1) __builtin_abort();

  p = __builtin_strcpy(tmp, str);
  if (__builtin_memcmp (p, str, sizeof(str))) __builtin_abort();

  p = __builtin_strncpy(tmp, str, sizeof(str));
  if (__builtin_memcmp (p, str, sizeof(str))) __builtin_abort();

  if (__builtin_strcmp (p, str)) __builtin_abort();

  if (__builtin_strncmp (p, str, sizeof(str))) __builtin_abort();

  tmp[0] = '\0';
  p = __builtin_strcat(tmp, str);
  if (__builtin_memcmp (p, str, sizeof(str))) __builtin_abort();

  if (__builtin_strchr(p, 'z') != &p[25]) __builtin_abort();

  p = __builtin_strdup (str);
  if (__builtin_memcmp (p, str, sizeof(str))) __builtin_abort();
  __builtin_free(p);

  p = __builtin_malloc (100);
  __builtin_memset(p, 0, 100);
  p = __builtin_realloc (p, 1000);
  __builtin_memset(p, 0, 1000);
  __builtin_free(p);

  p = __builtin_calloc(10, 10);
  __builtin_memset(p, 0, 100);
  __builtin_free(p);

#if defined(__i386__) || defined(__x86_64__)
  p = __builtin_alloca(100);
  __builtin_memset(p, 0, 100);
#endif
}

int tst(void)
{
  long value = 3;
  return -value;
}

void tst_compare(void)
{
  /* This failed on risc64 */
  if (tst() > 0) printf ("error\n");
}

#pragma pack(1)
struct S { int d:24; int f:14; } i, j;
#pragma pack()

void tst_pack (void)
{
  i.f = 5; j.f = 5;
  if (j.f != i.f) printf("error\n");
}

int
main (void)
{
  struct big_struct big;

  tst_shift();
  tst_void_ptr(&big.a[0], 0);
#if !defined(_WIN32)
  tst_const_addr();
#endif
  tst_zero_struct();
  tst_big(big);
  tst_adr(&sprintf);
  tst_builtin();
  tst_compare();
  tst_pack();
}
