extern int printf (const char *, ...);
extern void target(void);
extern void alias_for_target(void);
extern void asm_for_target(void);

void inunit2(void);

void inunit2(void)
{
  target();
  alias_for_target();
  /* This symbol is not supposed to be available in this unit:
     asm_for_target();
   */
}
