void pointer_parameters(int *const cp, int *volatile vp, int *restrict rp)
{
    _Static_assert(_Generic(&cp, int *const *: 1, default: 0),
                   "const pointer parameter remains qualified");
    _Static_assert(_Generic(&vp, int *volatile *: 1, default: 0),
                   "volatile pointer parameter remains qualified");
   _Static_assert(_Generic(&rp, int *restrict *: 1, default: 0),
                  "restrict pointer parameter remains qualified");
}

void array_parameters(int ca[const 2], int va[volatile 2], int ra[restrict 2])
{
    _Static_assert(_Generic(&ca, int *const *: 1, default: 0),
                   "const array parameter adjusts to qualified pointer");
    _Static_assert(_Generic(&va, int *volatile *: 1, default: 0),
                   "volatile array parameter adjusts to qualified pointer");
    _Static_assert(_Generic(&ra, int *restrict *: 1, default: 0),
                   "restrict array parameter adjusts to qualified pointer");
}

void declared_const(int *const pointer);
void declared_const(int *pointer)
{
    _Static_assert(_Generic(&pointer, int **: 1, default: 0),
                   "declaration qualifier does not affect definition");
    pointer = 0;
}

void defined_const(int *pointer);
void defined_const(int *const pointer)
{
    _Static_assert(_Generic(&pointer, int *const *: 1, default: 0),
                   "definition retains const");
}

void declared_volatile(int *volatile pointer);
void declared_volatile(int *pointer)
{
    _Static_assert(_Generic(&pointer, int **: 1, default: 0),
                   "declaration qualifier does not affect definition");
    pointer = 0;
}

void defined_volatile(int *pointer);
void defined_volatile(int *volatile pointer)
{
    _Static_assert(_Generic(&pointer, int *volatile *: 1, default: 0),
                   "definition retains volatile");
    pointer = 0;
}

void declared_restrict(int *restrict pointer);
void declared_restrict(int *pointer)
{
  _Static_assert(_Generic(&pointer, int **: 1, default: 0),
                 "declaration qualifier does not affect definition");
  pointer = 0;
}

void defined_restrict(int *pointer);
void defined_restrict(int *restrict pointer)
{
  _Static_assert(_Generic(&pointer, int *restrict *: 1, default: 0),
                 "definition retains restrict");
  pointer = 0;
}

int main(void)
{
    return 0;
}
