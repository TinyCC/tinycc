#if defined test_56_btype_excess_1
struct A {} int i;

#elif defined test_57_btype_excess_2
char int i;

#elif defined test_58_function_redefinition
int f(void) { return 0; }
int f(void) { return 1; }

#elif defined test_global_redefinition
int xxx = 1;
int xxx;
int xxx = 2;

#elif defined test_59_function_array
int (*fct)[42](int x);

#elif defined test_60_enum_redefinition
enum color { RED, GREEN, BLUE };
enum color { R, G, B };
enum color c;

#elif defined test_62_enumerator_redefinition
enum color { RED, GREEN, BLUE };
enum rgb { RED, G, B};
enum color c = RED;

#elif defined test_63_local_enumerator_redefinition
enum {
    FOO,
    BAR
};

int main(void)
{
    enum {
        FOO = 2,
        BAR
    };

    return BAR - FOO;
}

#elif defined test_61_undefined_enum
enum rgb3 c = 42;

#elif defined test_74_non_const_init
int i = i++;

#elif defined test_pointer_assignment

void (*f1)(void);
void f2(void) {}

struct s1 *ps1;
struct s2 *ps2;

void *v1, **v2, ***v3;

enum e1 { a = 4 } e10, *e11, *e12;
enum e2 { b = -4 } e20, *e21;
enum e3 { c = 5000000000LL } e30;

int *ip;
unsigned int *up;
long *lp;
long long *llp;

char **c1;
char const **c2;
unsigned char **u1;

int no_main ()
{
    // function
    f1 = f2;
    // struct
    ps1 = ps2;
    // void*
    v1 = v3;
    v2 = v3;

    // enum
    e11 = e12;
    e11 = e21;
    e11 = &e10;
    ip = &e10;
    ip = &e20;
    up = &e10;
    up = &e20;
    up = &e30;

    lp = ip;
    lp = llp;

    // constness
    c1 = c2;
    *c1 = *c2;
    **c1 = **c2;

    // unsigned = signed
    u1 = c2;
    *u1 = *c2;
    **u1 = **c2;

    c2 = c1;
    *c2 = *c1;
    **c2 = **c1;

    return 0;
}


#elif defined test_enum_compat
enum e4;
enum e5;
void f3(enum e4 e);
void f3(enum e5 e);

#elif defined test_ptr_to_str
void f() { _Generic((int const *[]){0}, int:0); }
#elif defined test_fnptr_to_str
void f() { _Generic((int (*(*)(float,char))(double,int)){0}, int:0); }
#elif defined test_array_to_str
void f() { _Generic((int(*)[3]){0}, int:0); }
#elif defined test_duplicate_def_1
static enum myenum { L = -1 } L;                                                
#elif defined test_duplicate_def_2
void foo(void) {
static enum myenum { L = -1 } L;                                                
}
#elif defined test_abstract_decls
int bar(const char *());     // abstract declarator here is okay
int bar (const char *(*g)()) // should match this 'g' argument
{
  g();
  return 42;
}
int foo(int ())              // abstract decl is wrong in definitions
{
  return 0;
#elif defined test_invalid_1
void f(char*);
void g(void) {
  f((char[]){1, ,});
}
#elif defined test_invalid_2
int ga = 0.42 { 2 };
#elif defined test_invalid_3
struct S { int a, b; };
struct T { struct S x; };
struct T gt = { 42 a: 1, 43 };
#elif defined test_conflicting_types
int i;
void foo(void) {
    int i;
      {
        extern double i;
        i = 42.2;
      }
}
#elif defined test_nested_types
union u {
    union u {
        int i;
    } m;
};
#elif defined test_vla_1
int X=1;

int main(void) {
  int t[][][X];
}
#elif defined test_invalid_alignas
/* _Alignas is no type qualifier */
void * _Alignas(16) p1;

#elif defined test_static_assert

#define ONE 0
 _Static_assert(ONE == 0, "don't show me this");
 _Static_assert(ONE == 1, "ONE is not 1");

#endif
