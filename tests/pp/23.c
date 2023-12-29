#define M_DEFERRED_COMMA            ,

#define M_EAT(...)
#define M_APPLY(a, ...)             a (__VA_ARGS__)
#define M_ID(...)                 __VA_ARGS__

#define M_C2I(a, ...)       a ## __VA_ARGS__
#define M_C(a, ...)         M_C2I(a, __VA_ARGS__)
#define M_C3I(a, b, ...)    a ## b ## __VA_ARGS__
#define M_C3(a, b, ...)     M_C3I(a ,b, __VA_ARGS__)
#define M_C4I(a, b, c, ...) a ## b ## c ## __VA_ARGS__
#define M_C4(a, b, c, ...)  M_C4I(a ,b, c, __VA_ARGS__)

#define M_INVI_0                    1
#define M_INVI_1                    0
#define M_INV(x)                    M_C(M_INVI_, x)

#define M_ANDI_00                   0
#define M_ANDI_01                   0
#define M_ANDI_10                   0
#define M_ANDI_11                   1
#define M_AND(x,y)                  M_C3(M_ANDI_, x, y)

#define M_ANDI_000                  0
#define M_ANDI_001                  0
#define M_ANDI_010                  0
#define M_ANDI_011                  0
#define M_ANDI_100                  0
#define M_ANDI_101                  0
#define M_ANDI_110                  0
#define M_ANDI_111                  1
#define M_AND3(x,y,z)               M_C4(M_ANDI_, x, y, z)

#define M_ORI_00                    0
#define M_ORI_01                    1
#define M_ORI_10                    1
#define M_ORI_11                    1
#define M_OR(x,y)                   M_C3(M_ORI_, x, y)

#define M_ORI_000                   0
#define M_ORI_001                   1
#define M_ORI_010                   1
#define M_ORI_011                   1
#define M_ORI_100                   1
#define M_ORI_101                   1
#define M_ORI_110                   1
#define M_ORI_111                   1
#define M_OR3(x,y,z)                M_C4(M_ORI_, x, y, z)

#define M_RETI_ARG2(_1, _2, ...) _2
#define M_RET_ARG2(...) M_RETI_ARG2(__VA_ARGS__,)

#define M_RETI_ARG4(_1, _2, _3, _4, ...) _4
#define M_RET_ARG4(...) M_RETI_ARG4(__VA_ARGS__,)

#define M_TOBOOLI_0                 1, 0,
#define M_BOOL(x)                   M_RET_ARG2(M_C(M_TOBOOLI_, x), 1, useless)

#define M_IFI_0(true_c, ...)        __VA_ARGS__
#define M_IFI_1(true_c, ...)        true_c
#define M_IF(c)                     M_C(M_IFI_, M_BOOL(c))

#define M_NARGS(...) M_RET_ARG4(__VA_ARGS__, 3, 2, 1, useless)

#define M_MAP(...)        M_MAP_0(__VA_ARGS__)
#define M_MAP_0(f, ...)   M_C(M_MAP_, M_NARGS(__VA_ARGS__))(f, __VA_ARGS__)
#define M_MAP_1(f, _1) f(_1) 
#define M_MAP_2(f, _1, _2) f(_1) f(_2) 
#define M_MAP_3(f, _1, _2, _3) f(_1) f(_2) f(_3) 

#define M_MAP2(...)        M_MAP2I_0(__VA_ARGS__)
#define M_MAP2I_0(f, d, ...)  M_C(M_MAP2I_, M_NARGS(__VA_ARGS__))(f, d, __VA_ARGS__)
#define M_MAP2I_1(f, d, _1) f(d, _1) 
#define M_MAP2I_2(f, d, _1, _2) f(d, _1) f(d, _2) 
#define M_MAP2I_3(f, d, _1, _2, _3) f(d, _1) f(d, _2) f(d, _3) 

#define M_CROSS_MAP(f, alist, blist)    M_CROSSI_MAP1(f, alist, blist)
#define M_CROSSI_MAP1(f, alist, blist)  M_C(M_CROSSI_MAP1_, M_NARGS alist)(f, blist, M_ID alist)
#define M_CROSSI_MAP1_1(...) M_CROSSI_MAP1_1b(__VA_ARGS__)
#define M_CROSSI_MAP1_1b(f, blist, a1) M_MAP2(f, a1, M_ID blist) 
#define M_CROSSI_MAP1_2(...) M_CROSSI_MAP1_2b(__VA_ARGS__)
#define M_CROSSI_MAP1_2b(f, blist, a1, a2) M_MAP2(f, a1, M_ID blist) M_MAP2(f, a2, M_ID blist) 
#define M_CROSSI_MAP1_3(...) M_CROSSI_MAP1_3b(__VA_ARGS__)
#define M_CROSSI_MAP1_3b(f, blist, a1, a2, a3) M_MAP2(f, a1, M_ID blist) M_MAP2(f, a2, M_ID blist) M_MAP2(f, a3, M_ID blist) 

#define M_COMMA_P(...)              M_RETI_ARG4(__VA_ARGS__, 1, 1, 0, useless)

#define M_PARENTHESISI_DETECT1(...)  ,
#define M_PARENTHESIS_P(...)                                              \
  M_AND3(M_COMMA_P(M_PARENTHESISI_DETECT1 __VA_ARGS__),                   \
         M_INV(M_COMMA_P(__VA_ARGS__)),                                   \
         M_EMPTY_P(M_EAT __VA_ARGS__))

#define M_EMPTYI_DETECT(...)        ,
#define M_EMPTYI_P_C1(...)          M_COMMA_P(M_EMPTYI_DETECT __VA_ARGS__ () )
#define M_EMPTYI_P_C2(...)          M_COMMA_P(M_EMPTYI_DETECT __VA_ARGS__)
#define M_EMPTYI_P_C3(...)          M_COMMA_P(__VA_ARGS__ () )
#define M_EMPTY_P(...)              M_AND(M_EMPTYI_P_C1(__VA_ARGS__), M_INV(M_OR3(M_EMPTYI_P_C2(__VA_ARGS__), M_COMMA_P(__VA_ARGS__),M_EMPTYI_P_C3(__VA_ARGS__))))

#define M_G3N_IS_PRESENT(num)                                              \
  M_IF(M_PARENTHESIS_P(M_C(M_GENERIC_ORG_, num)() ))(M_DEFERRED_COMMA M_ID M_C(M_GENERIC_ORG_, num)(), )

#define M_G3N_IS_PRESENT2(el1, num)                                        \
  M_IF(M_PARENTHESIS_P(M_C4(M_GENERIC_ORG_, el1, _COMP_, num)()))(M_DEFERRED_COMMA M_APPLY(M_C3, el1, _COMP_, M_ID M_C4(M_GENERIC_ORG_, el1, _COMP_, num)()), )

#define FLT1 (GENTYPE(float), TYPE(float) )

#define M_GENERIC_ORG_2() (USER)
#define M_GENERIC_ORG_USER_COMP_1() (CORE)
#define M_GENERIC_ORG_USER_COMP_CORE_OPLIST_6() FLT1

// Shall expand to ", USER"
M_MAP(M_G3N_IS_PRESENT, 1, 2, 3)
// Shall expand to ", USER_COMP_CORE"
M_CROSS_MAP(M_G3N_IS_PRESENT2, (MLIB , USER), ( 1, 2, 3 ) )
// Shall expand to ", USER_COMP_CORE" (composition of both) [fail]
M_CROSS_MAP(M_G3N_IS_PRESENT2, (MLIB M_MAP(M_G3N_IS_PRESENT, 1, 2, 3)), ( 1, 2, 3 ) )
