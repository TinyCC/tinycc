#include <stdio.h>

extern int Array3[10];
int Array3[] = { 12, 34, };
int main()
{
   int Count;

   int Array[10] = { 12, 34, 56, 78, 90, 123, 456, 789, 8642, 9753 };

   for (Count = 0; Count < 10; Count++)
      printf("%d: %d\n", Count, Array[Count]);

   int Array2[10] = { 12, 34, 56, 78, 90, 123, 456, 789, 8642, 9753, };

   for (Count = 0; Count < 10; Count++)
      printf("%d: %d\n", Count, Array2[Count]);

   for (Count = 0; Count < 10; Count++)
      printf("%d: %d\n", Count, Array3[Count]);

   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/
