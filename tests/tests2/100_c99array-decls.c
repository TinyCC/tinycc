void foo(int [5]);
void fooc(int x[const 5]);
void foos(int x[static 5]);
void foov(int x[volatile 5]);
void foor(int x[restrict 5]);
void fooc(int [const 5]);
void foos(int [static 5]);
void foov(int [volatile 5]);
void foor(int [restrict 5]);
void fooc(int (* const x));
void foos(int *x);
void foov(int * volatile x);
void foor(int * restrict x);
void fooc(int x[volatile 5])
{
  x[3] = 42;
#ifdef INVALID
  x = 0;
#endif
}
void foovm(int x[const *]);
void foovm(int * const x);

typedef int *pointer_array[2];
typedef int *restrict restricted_pointer;
typedef restrict pointer_array restricted_pointer_array;

_Static_assert(!__builtin_types_compatible_p(int **,
                                             restricted_pointer *),
               "nested restrict qualifier is retained");
_Static_assert(__builtin_types_compatible_p(restricted_pointer_array,
                                            restricted_pointer[2]),
               "restrict can qualify an array of pointer types");

restricted_pointer *restricted_pp;
int **unrestricted_pp;

_Static_assert(__builtin_types_compatible_p(
                   __typeof__(1 ? restricted_pp : unrestricted_pp),
                   restricted_pointer *),
               "conditional expression combines restrict qualifiers");

#ifdef INVALID
void wrongc(int x[3][const 4]);
void wrongvm(int x[static *]);
void foovm(int x[const *])
{
  x[2] = 1;
}
#endif
int main()
{
  return 0;
}

void vla_compat(int n, int (*pai)[n], const int (*pcai)[n], int (*pa4i)[4])
{
    _Static_assert(__builtin_types_compatible_p(int[n], int[4]),
                   "variable and fixed bounds are compatible");
    _Static_assert(__builtin_types_compatible_p(typeof(n ? pcai : pai), typeof(pcai)),
                   "conditional transmits const to VLA elements");
    _Static_assert(sizeof *(n ? pa4i : pai) == 4 * sizeof(int),
                   "conditional retains the known constant bound");
}

typedef int Unknown[], Seven[7];

void nested_bounds(int c, Unknown *(*p)[4], Seven *(*q)[])
{
    typeof(c ? p : q) pq;
    typeof(c ? q : p) qp;
    _Static_assert(sizeof *pq == 4 * sizeof (*pq)[0], "outer bound from p");
    _Static_assert(sizeof *qp == sizeof *pq, "outer bound in reverse order");
    _Static_assert(sizeof *(*pq)[0] == sizeof(Seven), "inner bound from q");
    _Static_assert(sizeof *(*qp)[0] == sizeof(Seven), "inner bound in reverse order");
}
