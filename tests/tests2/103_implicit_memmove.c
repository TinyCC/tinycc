/* Test that the memmove TCC is emitting for the struct copy
   and hence implicitely declares can be declared properly also
   later.  */
struct S { int a,b,c,d, e[1024];};
int foo (struct S *a, struct S *b)
{
  *a = *b;
  return 0;
}

void *memmove(void*,const void*,__SIZE_TYPE__);
void foo2 (struct S *a, struct S *b)
{
  memmove(a, b, sizeof *a);
}

int main()
{
  return 0;
}
