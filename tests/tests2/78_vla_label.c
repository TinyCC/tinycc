#include <stdio.h>

/* This test segfaults as of April 27, 2015. */

void f(int argc)
{
  char test[argc];
  if(0)
  label:
    printf("boom!\n");
  if(argc-- == 0)
    return;
  goto label;
}

int main()
{
  f(2);

  return 0;
}
