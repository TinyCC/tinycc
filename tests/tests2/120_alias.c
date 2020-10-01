/* Check semantics of various constructs to generate renamed symbols.  */
extern int printf (const char *, ...);
void target(void);
void target(void) {
    printf("in target function\n");
}

void alias_for_target(void) __attribute__((alias("target")));
#ifdef __leading_underscore
void asm_for_target(void) __asm__("_target");
#else
void asm_for_target(void) __asm__("target");
#endif

/* This is not supposed to compile, alias targets must be defined in the
   same unit.  In TCC they even must be defined before the reference
void alias_for_undef(void) __attribute__((alias("undefined")));
*/

extern void inunit2(void);

int main(void)
{
  target();
  alias_for_target();
  asm_for_target();
  inunit2();
  return 0;
}
