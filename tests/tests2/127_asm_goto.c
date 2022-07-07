static int simple_jump(void)
{
  asm goto ("jmp %l[label]" : : : : label);
  return 0;
label:
  return 1;
}

static int three_way_jump(int val, int *addr)
{
  *addr = 42;
  asm goto ("cmp $0, %1\n\t"
            "jg %l[larger]\n\t"
            "jl %l[smaller]\n\t"
            "incl %0\n\t"
            : "=m" (*addr)
            : "r" (val)
            :
            : smaller, larger);
  return 1;
smaller:
  return 2;
larger:
  return 3;
}

static int another_jump(void)
{
  asm goto ("jmp %l[label]" : : : : label);
  return 70;
  /* Use the same label name as in simple_jump to check that
     that doesn't confuse our C/ASM symbol tables */
label:
  return 71;
}

extern int printf (const char *, ...);
int main(void)
{
  int i;
  if (simple_jump () == 1)
    printf ("simple_jump: okay\n");
  else
    printf ("simple_jump: wrong\n");
  if (another_jump () == 71)
    printf ("another_jump: okay\n");
  else
    printf ("another_jump: wrong\n");
  if (three_way_jump(0, &i) == 1 && i == 43)
    printf ("three_way_jump(0): okay\n");
  else
    printf ("three_way_jump(0): wrong (i=%d)\n", i);
  if (three_way_jump(1, &i) == 3 && i == 42)
    printf ("three_way_jump(1): okay\n");
  else
    printf ("three_way_jump(1): wrong (i=%d)\n", i);
  if (three_way_jump(-1, &i) == 2 && i == 42)
    printf ("three_way_jump(-1): okay\n");
  else
    printf ("three_way_jump(-1): wrong (i=%d)\n", i);
  return 0;
}
