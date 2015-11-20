#include <stdio.h>

/* This test segfaults as of April 27, 2015. */
void f1(int argc)
{
  char test[argc];
  if(0)
  label:
    printf("boom!\n");
  if(argc-- == 0)
    return;
  goto label;
}

/* This segfaulted on 2015-11-19. */
void f2(void)
{
    goto start;
    {
        int a[1 ? 1 : 1]; /* not a variable-length array */
    start:
        a[0] = 0;
    }
}

int main()
{
  f1(2);
  f2();

  return 0;
}
