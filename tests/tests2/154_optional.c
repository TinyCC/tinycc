typedef int Fn(double), Array2[2], Array2x3[2][3];

/* 6.2.1.1 and 6.4.1.1: conversions, including generic controlling expressions. */
_Optional int *poi, **ppoi;
int *pi;
_Optional const Array2 *pocai;
Array2 *pai;
_Optional Array2 *poai;
_Optional Fn *pofri;
Fn *pfri;

_Static_assert(_Generic(*poi, int: 1, default: 0),
               "lvalue conversion yields int");
_Static_assert(_Generic(*pocai, const int *: 1, default: 0),
               "array conversion removes optional but preserves const");
_Static_assert(_Generic(*pofri, Fn *: 1, default: 0),
               "function conversion removes optional");
_Static_assert(__builtin_types_compatible_p(_Optional int, int),
               "type compatibility ignores top-level optional");
_Static_assert(!__builtin_types_compatible_p(_Optional int *, int *),
               "type compatibility preserves referenced optional");

/* 6.3.2.1: macro replacement precedes keyword recognition. */
#define _Optional
/* Valid: macro expansion removes the qualifier, leaving int i_macro. */
_Optional int i_macro;
#undef _Optional

/* 6.4.2.1--6.4.2.4: postfix operators and aggregate propagation. */
struct S {
    int i, ai[2];
};
_Optional const struct S *pocs;

_Static_assert(_Generic(pocs->i, int: 1, default: 0),
               "member lvalue conversion");
_Static_assert(_Generic(&pocs->i, const int *: 1, default: 0),
               "member address removes optional but preserves const");
_Static_assert(_Generic(pocs->ai, const int *: 1, default: 0),
               "member array conversion removes optional");
_Static_assert(_Generic(poi++, _Optional int *: 1, default: 0),
               "postfix increment preserves operand type");
_Static_assert(_Generic(poi--, _Optional int *: 1, default: 0),
               "postfix decrement preserves operand type");

/* 6.4.2.5: only the compound literal's top-level type is constrained. */
_Static_assert(_Generic((_Optional int *){0}, _Optional int *: 1, default: 0),
               "pointer compound literal is valid");

/* 6.4.3.1--6.4.3.2: prefix and address operators. */
_Static_assert(_Generic(++poi, _Optional int *: 1, default: 0),
               "prefix increment preserves operand type");
_Static_assert(_Generic(--poi, _Optional int *: 1, default: 0),
               "prefix decrement preserves operand type");
_Static_assert(_Generic(&*poi, int *: 1, default: 0),
               "address removes optional");
_Static_assert(_Generic(&*pocai, const Array2 *: 1, default: 0),
               "address removes optional from array type");
_Static_assert(_Generic(&*pofri, Fn *: 1, default: 0),
               "address removes optional from function type");

/* 6.4.3.3--6.4.4: representation and casts. */
_Static_assert(sizeof(_Optional int) == sizeof(int),
               "optional does not change size");
_Static_assert(_Alignof(_Optional int) == _Alignof(int),
               "optional does not change alignment");
_Static_assert(_Generic((_Optional int)3, int: 1, default: 0),
               "cast result is unqualified");
_Static_assert(_Generic((int *)poi, int *: 1, default: 0),
               "explicit pointer cast removes optional");

/* 6.4.5: additive operators remove optional from the target type. */
_Static_assert(_Generic(poi + 1, int *: 1, default: 0),
               "addition removes optional");
_Static_assert(_Generic(poi + 0, int *: 1, default: 0),
               "addition of 0 removes optional");
_Static_assert(_Generic(1 + poi, int *: 1, default: 0),
               "reverse addition removes optional");
_Static_assert(_Generic(0 + poi, int *: 1, default: 0),
               "reverse addition of 0 removes optional");
_Static_assert(_Generic(poi - 1, int *: 1, default: 0),
               "subtraction removes optional");
_Static_assert(_Generic(poi - 0, int *: 1, default: 0),
               "subtraction of 0 removes optional");
_Static_assert(_Generic(poi += 0, _Optional int *: 1, default: 0),
               "compound addition of 0 preserves optional");
_Static_assert(_Generic(poi -= 0, _Optional int *: 1, default: 0),
               "compound subtraction of 0 preserves optional");

/* 6.4.7: conditional pointer composition combines qualifiers. */
const void *pcv;

_Static_assert(_Generic(1 ? poi : pi, _Optional int *: 1, default: 0),
               "conditional transmits optional object qualification");
_Static_assert(_Generic(1 ? pcv : poi, const _Optional void *: 1, default: 0),
               "conditional combines qualifiers");
_Static_assert(_Generic(1 ? pai : poai, _Optional Array2 *: 1, default: 0),
               "conditional transmits optional array qualification");
_Static_assert(_Generic(1 ? pfri : pofri, _Optional Fn *: 1, default: 0),
               "conditional transmits optional function qualification");

/* 6.5.2.2: typeof preserves optional qualification. */
typeof(_Optional int) *poi_type;
typeof(*poi) *poi_expr;

_Static_assert(_Generic(poi_type, _Optional int *: 1, default: 0),
               "typeof type name preserves optional");
_Static_assert(_Generic(poi_expr, _Optional int *: 1,
                        default: 0),
               "typeof expression preserves optional");

#define DECL_PTR(T, N) \
    static typeof(T) *N; \
    static void set_##N(typeof(T) *p) \
    { \
        N = p; \
    }

DECL_PTR(_Optional int, poi_macro)

#define optional_cast(p) ((typeof(&*(p)))(p))

_Static_assert(_Generic(optional_cast(pocs), const struct S *: 1, default: 0),
               "optional_cast removes only optional");

/* 6.5.4.2 and 6.5.5: arrays and typedefs. */
typedef _Optional int OptionalInt, *PointerToOptional;
typedef OptionalInt OptionalArray2x3[2][3];
typedef _Optional Array2x3 OptionalArray2x3Alt;
typedef int *_Optional OptionalPointer,
    (*_Optional OptionalFunctionPointer)(double);
typedef _Optional Fn *PointerToOptionalFunction;

struct PointerMembers {
    _Optional int *poi, (*poai)[2];
    _Optional Fn *pofri;
};

void pointer_parameters(_Optional int *poi,
                        _Optional int (*poaai)[2][3],
                        _Optional Fn *pofri);

OptionalArray2x3 *paaoi;
OptionalInt (*paaoi_alt)[2][3];
OptionalArray2x3Alt *poaai;
OptionalPointer *popi;
PointerToOptional poi_typedef;
OptionalFunctionPointer *popfri;
PointerToOptionalFunction pofri_typedef;

_Static_assert(_Generic(paaoi, typeof(poaai): 1, default: 0),
               "qualifying array and element type is equivalent");
_Static_assert(_Generic(poi_typedef, _Optional int *: 1, default: 0),
               "optional object typedef");
_Static_assert(_Generic(*pofri_typedef, Fn *: 1, default: 0),
               "optional function typedef conversion");

/* 6.5.4.3: return qualifiers and adjusted parameters. */
_Optional int qualified_return(double);
_Static_assert(_Generic(qualified_return, int (*)(double): 1, default: 0),
               "function return qualifiers are removed");
_Optional int *return_optional_object(double),
    (*return_optional_array(void))[2];
_Optional Fn *return_optional_function(void);

static void adjusted_array(_Optional int poai[2][3])
{
    _Static_assert(_Generic(poai, _Optional int (*)[3]: 1, default: 0),
                   "array parameter adjustment");
}

static void adjusted_array_typedef(OptionalArray2x3 poai)
{
    _Static_assert(_Generic(poai, _Optional int (*)[3]: 1, default: 0),
                   "array typedef parameter adjustment");
}

static void adjusted_static_array(_Optional int poi[static 4])
{
    (void)poi;
}

static void adjusted_function(_Optional Fn pofri)
{
    _Static_assert(_Generic(pofri, _Optional Fn *: 1, default: 0),
                   "function parameter adjustment");
}

typedef _Optional Fn OptionalFn;

static void adjusted_function_typedef(OptionalFn pofri)
{
    _Static_assert(_Generic(pofri, _Optional Fn *: 1, default: 0),
                   "function typedef parameter adjustment");
}

/* Compiled with -Wall -Werror: adding optional must not produce a diagnostic. */
static void assignment_adds_qualifier(_Optional int *poi, int *pi)
{
    poi = pi;
    (void)poi;
}

/* SDCC #4164: array typedef qualifiers apply to pointer elements. */
typedef int *PointerArray[3];
typedef PointerArray PointerMatrix[2];
typedef int *_Optional OptionalPointerArray[3];
typedef OptionalPointerArray OptionalPointerMatrix[2];
typedef _Optional int *OptionalTargetArray[3],
    *const ConstOptionalTargetArray[3];

_Static_assert(__builtin_types_compatible_p(_Optional PointerArray *,
                                           OptionalPointerArray *),
               "optional qualifies pointer array elements");
_Static_assert(__builtin_types_compatible_p(PointerArray _Optional *,
                                           OptionalPointerArray *),
               "optional after array typedef qualifies elements");
_Static_assert(__builtin_types_compatible_p(_Optional PointerMatrix *,
                                           OptionalPointerMatrix *),
               "optional qualifies multidimensional pointer array elements");
_Static_assert(__builtin_types_compatible_p(const OptionalTargetArray *,
                                           ConstOptionalTargetArray *),
               "const on array typedef preserves optional pointer targets");

/* Aggregate scalar members inherit qualification at their own type level. */
_Static_assert(__builtin_types_compatible_p(typeof(pocs->i) *,
                                           const _Optional int *),
               "aggregate scalar member inherits optional and const");
_Static_assert(__builtin_types_compatible_p(_Optional const volatile restrict PointerMatrix *,
                                           int *const volatile restrict _Optional (*)[2][3]),
               "array typedef qualifiers apply to pointer elements");

/* Optional union member decay must preserve inherited volatile. */
union ArrayUnion {
    char ac[2];
    int i;
};
_Optional volatile union ArrayUnion *povu;
_Static_assert(_Generic(povu->ac,
                        volatile char *: 1, default: 0),
               "optional union member decay preserves volatile");

/* SDCC #4005: decay removes only optional qualification of the element. */
struct ArrayMembers {
    char *restrict arpc[2];
    _Optional char *apoc[2];
};
_Optional const volatile struct ArrayMembers *pocvs;

_Static_assert(_Generic(pocvs->arpc,
                        char *const volatile restrict *: 1, default: 0),
               "member array decay preserves const volatile restrict");
_Static_assert(_Generic(pocvs->apoc,
                        _Optional char *const volatile *: 1, default: 0),
               "member array decay preserves nested optional");
_Static_assert(_Generic(*paaoi, int (*)[3]: 1, default: 0),
               "multidimensional array decay removes optional");
_Static_assert(_Generic(&*paaoi, Array2x3 *: 1, default: 0),
               "array address removes optional through all dimensions");
_Static_assert(_Generic(paaoi + 0, Array2x3 *: 1, default: 0),
               "array pointer arithmetic removes optional");
_Static_assert(_Generic(poi, _Optional int *: 1, default: 0),
               "arithmetic does not change the original pointer type");
_Static_assert(_Generic(paaoi, OptionalArray2x3 *: 1, default: 0),
               "conversions do not change shared array types");
_Static_assert(_Generic(poai, _Optional Array2 *: 1, default: 0),
               "conditional composition does not change its operands");

/* SDCC #3028: function parameter adjustment also applies to prototypes. */
void function_parameter_first(_Optional Fn pofri);
void function_parameter_first(_Optional Fn *pofri);
void pointer_parameter_first(_Optional Fn *pofri);
void pointer_parameter_first(_Optional Fn pofri);
typedef void FunctionParameter(_Optional Fn);
_Static_assert(__builtin_types_compatible_p(FunctionParameter *,
                                           void (*)(_Optional Fn *)),
               "unnamed optional function parameter adjustment");

/* A sizeof operand can contain nested optional function types. */
_Static_assert(sizeof((struct { _Optional Fn *pofri; }){0}) == sizeof(Fn *),
               "sizeof compound literal containing optional function pointer");

/* Actual return conversions also pass through assignment checking. */
static int *array_decay(_Optional int (*poai)[2])
{
    return *poai;
}

static Fn *function_decay(_Optional Fn *pofri)
{
    return *pofri;
}

int main(void)
{
    (void)i_macro;
    (void)ppoi;
    (void)paaoi_alt;
    (void)popi;
    (void)popfri;
    (void)set_poi_macro;
    (void)adjusted_array;
    (void)adjusted_static_array;
    (void)adjusted_array_typedef;
    (void)adjusted_function;
    (void)adjusted_function_typedef;
    (void)assignment_adds_qualifier;
    (void)array_decay;
    (void)function_decay;
    return 0;
}
