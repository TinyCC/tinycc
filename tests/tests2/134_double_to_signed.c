#include <stdio.h>
int main() {
  printf("%d\n", (int)-1.0);
  double d = -1.0;
  printf("%d\n", (int)d);

  printf("%d\n", (int)-2147483648.0);
  d = -2147483648.0;
  printf("%d\n", (int)d);
}
