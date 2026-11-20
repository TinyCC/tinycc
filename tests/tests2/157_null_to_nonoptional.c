#if defined test_default
int *pi = 0;

#elif defined test_conversions
#pragma comment(option, "-Wnull-to-nonoptional")
int *pi = 0; /* warn */
void accept(int *);
int *function(void)
{
    int **ppi = (void *)0; /* warn */
    pi = 0; /* warn */
    accept((void *)0); /* warn */
    struct { int *pi; } s = {0}; /* warn */
    (void)ppi;
    (void)s;
    return 0; /* warn */
}

#elif defined test_optional
#pragma comment(option, "-Werror=null-to-nonoptional")
_Optional int *poi = 0;
void accept(_Optional int *);
_Optional int *function(int i, int *pi)
{
    poi = i ? pi : (void *)0;
    accept(i ? 0 : pi);
    return i ? pi : 0;
}

#elif defined test_conditional
#pragma comment(option, "-Wnull-to-nonoptional")
void accept(int *);
int *function(int i, int j, int *pi)
{
    pi = i ? (j ? pi : 0) : (j ? pi : (void *)0); /* warn */
    accept((0, i ? pi : 0)); /* warn */
    return _Generic(i, int: i ? pi : 0); /* warn */
}

#elif defined test_function_pointer
#pragma comment(option, "-Wnull-to-nonoptional")
typedef void Fn(void);
Fn *function(int i, Fn *pf)
{
    return i ? pf : 0; /* warn */
}

#elif defined test_no_conversion
#pragma comment(option, "-Werror=null-to-nonoptional")
void accept(int *);
void function(int i, int j, int *pi)
{
    pi = (int *)(i ? pi : 0);
    pi = i ? (int *)0 : pi;
    accept(_Generic(i, int: pi, default: i ? pi : 0));
    accept(__builtin_choose_expr(1, pi, i ? pi : 0));
    accept(((void)(i ? pi : 0), pi));
    (void)((i ? pi : 0) == (void *)0);
    (void)(j ? pi : 0);
    accept((void *)(int *)0);
    accept((void *)(unsigned long)(int *)0);
}

#elif defined test_cast_arm
#pragma comment(option, "-Wnull-to-nonoptional")
int *function(int i, int j, int *pi)
{
    return i ? (int *)0 : (j ? pi : 0); /* warn */
}

#elif defined test_indirection
#pragma comment(option, "-Werror=null-to-nonoptional")
void accept(int *);
void function(int i, int **ppi)
{
    if (i)
        accept(*(i ? ppi : 0));
    accept(i ? &*(i ? *ppi : 0) : *ppi);
}
#endif
