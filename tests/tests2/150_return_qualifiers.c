const int const_return(void);
volatile int volatile_return(void);
int *restrict restrict_return(void);

_Static_assert(__builtin_types_compatible_p(__typeof__(const_return()) *, int *),
               "const is removed from a return type");
_Static_assert(__builtin_types_compatible_p(__typeof__(volatile_return()) *, int *),
               "volatile is removed from a return type");
_Static_assert(__builtin_types_compatible_p(__typeof__(restrict_return()) *, int **),
               "restrict is removed from a return type");

int const_return(void)
{
   return 0;
}

int volatile_return(void)
{
   return 0;
}

int *restrict_return(void)
{
   return 0;
}

int main(void)
{
   return 0;
}
