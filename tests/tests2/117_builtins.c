#include <stdio.h>

struct big_struct { char a[262144]; };

static const char str[] = "abcdefghijklmnopqrstuvwxyz";

int
main (void)
{
  char *p;
  char tmp[100];
  int r = 0;

#if defined __BOUNDS_CHECKING_ON || defined BC_ON
  printf("BOUNDS ON:\n");
#else
  printf("BOUNDS OFF:\n");
#endif

  if (r != 0)
    __builtin_abort();

  r = (__builtin_offsetof(struct big_struct, a) != 0);
  printf(" 1:%d", !r);

  p = __builtin_memcpy (tmp, str, sizeof(str));
  r = (p != tmp);
  printf(" 2:%d", !r);

  r = __builtin_memcmp (p, str, sizeof(str));
  printf(" 3:%d", !r);

  p = __builtin_memmove(tmp, str, sizeof(str));
  r = (__builtin_memcmp (p, str, sizeof(str)));
  printf(" 4:%d", !r);

  p = __builtin_memset(tmp, 0, sizeof (tmp));
  r = (p != tmp || tmp[0] != 0 || tmp[99] != 0);
  printf(" 5:%d", !r);

  r = (__builtin_strlen(str) != sizeof(str) - 1);
  printf(" 6:%d", !r);

  p = __builtin_strcpy(tmp, str);
  r = (__builtin_memcmp (p, str, sizeof(str)));
  printf(" 7:%d", !r);

  p = __builtin_strncpy(tmp, str, sizeof(str));
  r = (__builtin_memcmp (p, str, sizeof(str)));
  printf(" 8:%d", !r);

  r = (__builtin_strcmp (p, str));
  printf(" 9:%d", !r);

  r = (__builtin_strncmp (p, str, sizeof(str)));
  printf(" 10:%d", !r);

  tmp[0] = '\0';
  p = __builtin_strcat(tmp, str);
  r = (__builtin_memcmp (p, str, sizeof(str)));
  printf(" 11:%d", !r);

  tmp[0] = '\0';
  p = __builtin_strncat(tmp, str, __builtin_strlen(str));
  r = (__builtin_memcmp (p, str, sizeof(str)));
  printf(" 12:%d", !r);

  r = (__builtin_strchr(p, 'z') != &p[25]);
  printf(" 13:%d", !r);

  r = (__builtin_strrchr(p, 'z') != &p[25]);
  printf(" 14:%d", !r);

  p = __builtin_strdup (str);
  r = (__builtin_memcmp (p, str, sizeof(str)));
  printf(" 15:%d", !r);
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
  printf("\n");
}
