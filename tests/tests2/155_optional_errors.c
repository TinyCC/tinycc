#if defined test_optional_keyword_identifier
int _Optional = 43; /* error */

#elif defined test_optional_obj
_Optional int oi; /* error */

#elif defined test_optional_pointer_obj
int *_Optional opi; /* error */

#elif defined test_obj_with_optional_array_type
_Optional int aaoi[2][3]; /* error */

#elif defined test_obj_with_array_of_optional_type_via_typedef
typedef _Optional int OptionalArray[2][3];
OptionalArray aaoi; /* error */

#elif defined test_obj_with_optional_array_type_via_typedef
typedef int Array[2][3];
typedef _Optional Array OptionalArray;
OptionalArray oaai; /* error */

#elif defined test_member_with_optional_type
struct S {
    _Optional int oi; /* error */
};

#elif defined test_member_with_optional_array_type
struct S {
    _Optional int aoi[2]; /* error */
};

#elif defined test_member_with_optional_type_via_typedef
typedef _Optional int OptionalInt;
struct S {
    OptionalInt oi; /* error */
};

#elif defined test_member_with_array_of_optional_type_via_typedef
typedef _Optional int OptionalArray[2];
struct S {
    OptionalArray aoi; /* error */
};

#elif defined test_union_member_with_optional_type
union U {
    _Optional int oi; /* error */
};

#elif defined test_optional_param
void function(_Optional int oi); /* error */

#elif defined test_optional_pointer_param
void function(int *_Optional opi); /* error */

#elif defined test_optional_pointer_to_array_param
void function(int (*_Optional opaai)[2][3]); /* error */

#elif defined test_optional_param_via_typedef
typedef _Optional int OptionalInt;
void function(OptionalInt oi); /* error */

#elif defined test_optional_compound_literal
void function(void)
{
    (_Optional int){0}; /* error */
}

#elif defined test_optional_pointer_compound_literal
void function(void)
{
    (int *_Optional){0}; /* error */
}

#elif defined test_optional_qualifier_loss
void function(_Optional int *poi)
{
    int *pi = poi; /* warn */
    (void)pi;
}

#elif defined test_optional_qualifier_loss_after_guard
void function(_Optional int *poi)
{
    int *pi;
    if (poi)
        pi = poi; /* warn */
    (void)pi;
}

#elif defined test_optional_qualifier_loss_nested
void function(_Optional int **ppoi)
{
    int **ppi = ppoi; /* warn */
    (void)ppi;
}

#elif defined test_optional_qualifier_loss_func
typedef int Function(double);
void function(_Optional Function *pofri)
{
    Function *pfri = pofri; /* warn */
    (void)pfri;
}

#elif defined test_optional_qualifier_loss_after_postincrement
void accept(int *);
void function(_Optional int *poi)
{
    accept(poi++); /* warn */
}

#elif defined test_optional_qualifier_loss_after_postdec
void accept(int *);
void function(_Optional int *poi)
{
    accept(poi--); /* warn */
}

#elif defined test_optional_qualifier_loss_after_preinc
void accept(int *);
void function(_Optional int *poi)
{
    accept(++poi); /* warn */
}

#elif defined test_optional_qualifier_loss_after_predecrement
void accept(int *);
void function(_Optional int *poi)
{
    accept(--poi); /* warn */
}

#elif defined test_optional_qualifier_loss_after_add_assignment
void function(_Optional int *poi)
{
    int *pi;
    pi = poi += 1; /* warn */
    (void)pi;
}

#elif defined test_optional_qualifier_loss_after_sub_assignment
void function(_Optional int *poi)
{
    int *pi;
    pi = poi -= 1; /* warn */
    (void)pi;
}

#elif defined test_optional_qualifier_loss_after_add_0_assignment
void function(_Optional int *poi)
{
    int *pi;
    pi = poi += 0; /* warn */
    (void)pi;
}

#elif defined test_optional_qualifier_loss_after_sub_0_assignment
void function(_Optional int *poi)
{
    int *pi;
    pi = poi -= 0; /* warn */
    (void)pi;
}

#elif defined test_optional_obj_via_typedef
typedef _Optional int OptionalInt;
OptionalInt oi; /* error */

#elif defined test_obj_with_optional_pointer_type_via_typedef
typedef int *_Optional OptionalPointer;
OptionalPointer opi; /* error */

#elif defined test_optional_func_via_typedef
typedef int Function(double);
typedef _Optional Function OptionalFunction;
OptionalFunction ofri; /* error */

#elif defined test_optional_func_declaration
_Optional typeof(int (double)) ofri; /* error */

#elif defined test_member_with_optional_type_in_typedef
typedef struct {
    _Optional int oi; /* error */
} InvalidStruct;

#elif defined test_optional_param_in_typedef
typedef void InvalidFunction(_Optional int oi); /* error */

#elif defined test_optional_array_param_qualifier
void function(int opi[_Optional 2]); /* error */

/* SDCC #4004: argument conversions must retain referenced qualifiers. */
#elif defined test_optional_qualifier_loss_void_argument
void accept(void *);
void function(_Optional int *poi)
{
    accept(poi); /* warn */
}

#elif defined test_optional_qualifier_loss_after_array_decay_argument
void accept(char **);
void function(void)
{
    _Optional char *apoc[2] = {0};
    accept(apoc); /* warn */
}

#elif defined test_optional_param_type_mismatch
void accept(void (*pfrv)(_Optional int *));
void function(void (*pfrv)(int *))
{
    accept(pfrv); /* warn */
}

#elif defined test_optional_return_type_mismatch
void accept(int *(*pfrpi)(void));
void function(_Optional int *(*pfrpoi)(void))
{
    accept(pfrpoi); /* warn */
}

/* SDCC #4006: array decay must preserve nested optional qualification. */
#elif defined test_optional_qualifier_loss_after_array_decay_initialisation
void function(void)
{
    _Optional char *apoc[2] = {0};
    char **ppc = apoc; /* warn */
    (void)ppc;
}

#elif defined test_optional_qualifier_loss_after_array_decay_assignment
void function(void)
{
    _Optional char *apoc[2] = {0};
    char **ppc;
    ppc = apoc; /* warn */
    (void)ppc;
}

#elif defined test_optional_qualifier_loss_after_array_decay_return
char **function(void)
{
    static _Optional char *apoc[2];
    return apoc; /* warn */
}

#elif defined test_optional_qualifier_loss_after_member_access
struct S { _Optional char *apoc[2]; };
char **function(struct S *ps)
{
    return ps->apoc; /* warn */
}

#elif defined test_cv_qualifier_loss_after_optional_array_decay
char *function(_Optional const volatile char (*pocvac)[2])
{
    return *pocvac; /* warn */
}

#elif defined test_cvr_qualifier_loss_after_optional_member_access
struct S { char *restrict arpc[2]; };
char **function(_Optional const volatile struct S *pocvs)
{
    return pocvs->arpc; /* warn */
}

/* Arithmetic removes optional while preserving const. */
#elif defined test_const_qualifier_loss_after_optional_arithmetic
char *function(_Optional const char *pocc)
{
    return pocc + 0; /* warn */
}

#endif
