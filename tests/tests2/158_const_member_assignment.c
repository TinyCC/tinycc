#if defined test_direct_member
struct S { const int value; };
void f(struct S *a, struct S *b)
{
    *a = *b; /* warn */
}

#elif defined test_nested_member
struct Inner { const int value; };
struct Outer { struct Inner inner; };
void f(struct Outer *a, struct Outer *b)
{
    *a = *b; /* warn */
}

#elif defined test_array_member
struct S { const int values[2]; };
void f(struct S *a, struct S *b)
{
    *a = *b; /* warn */
}

#elif defined test_array_of_struct
struct Inner { const int value; };
struct Outer { struct Inner entries[2][3]; };
void f(struct Outer *a, struct Outer *b)
{
    *a = *b; /* warn */
}

#elif defined test_union_member
union U { int value; const int readonly; };
void f(union U *a, union U *b)
{
    *a = *b; /* warn */
}

#elif defined test_nested_union
union U { int value; const int readonly; };
struct S { union U member; };
void f(struct S *a, struct S *b)
{
    *a = *b; /* warn */
}

#elif defined test_const_destination
struct S { int value; };
void f(const struct S *a, struct S *b)
{
    *a = *b; /* warn, once only */
}

#elif defined test_initialisation
struct S { const int value; };
void f(void)
{
    struct S a = {1};
    struct S b = a;
    (void)b;
}

#elif defined test_pointer_to_const
struct S { const int *pointer; };
void f(struct S *a, struct S *b)
{
    *a = *b;
}

#elif defined test_mutable_member
struct Inner { int value; };
struct Outer { struct Inner inner[2]; };
void f(struct Outer *a, struct Outer *b)
{
    *a = *b;
}

#elif defined test_cached_const
struct Inner { const int value; };
struct Outer { struct Inner elements[3]; };
void f(struct Outer *a, struct Outer *b)
{
    *a = *b;
    *a = *b;
}

#elif defined test_cached_mutable
struct Inner { int value; };
struct Outer { struct Inner elements[3]; };
void f(struct Outer *a, struct Outer *b)
{
    *a = *b;
    *a = *b;
}

#elif defined test_const_struct_member_not_cached
struct Inner { int value; };
struct Outer { const struct Inner member; };
void f(struct Outer *a, struct Outer *b, struct Inner *x, struct Inner *y)
{
    *a = *b;
    *x = *y;
    *a = *b;
    *x = *y;
}

#endif
