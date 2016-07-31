typedef unsigned char u8;
typedef struct {} empty_s;
struct contains_empty {
    u8 a;
    empty_s empty;
    u8 b;
};
struct contains_empty ce = { { (1) }, (empty_s){}, 022, };
/* The following decl of 'q' would demonstrate the TCC bug in init_putv when
   handling copying compound literals.  (Compound literals
   aren't acceptable constant initializers in isoc99, but
   we accept them like gcc, except for this case)
//char *q = (char *){ "trara" }; */
struct SS {u8 a[3], b; };
struct SS sinit16[] = { { 1 }, 2 };
struct S
{
  u8 a,b;
  u8 c[2];
};

struct T
{
  u8 s[16];
  u8 a;
};

struct U
{
  u8 a;
  struct S s;
  u8 b;
  struct T t;
};

struct V
{
  struct S s;
  struct T t;
  u8 a;
};

struct W
{
  struct V t;
  struct S s[];
};

struct S gs = ((struct S){1, 2, 3, 4});
struct S gs2 = {1, 2, {3, 4}};
struct T gt = {"hello", 42};
struct U gu = {3, 5,6,7,8, 4, "huhu", 43};
struct U gu2 = {3, {5,6,7,8}, 4, {"huhu", 43}};
/* Optional braces around scalar initializers.  Accepted, but with
   a warning.  */
struct U gu3 = { {3}, {5,6,7,8,}, 4, {"huhu", 43}};
/* Many superfluous braces and leaving out one initializer for U.s.c[1] */
struct U gu4 = { 3, {5,6,7,},  5, { "bla", {44}} };
/* Superfluous braces and useless parens around values */
struct S gs3 = { (1), {(2)}, {(((3))), {4}}};
/* Superfluous braces, and leaving out braces for V.t, plus cast */
struct V gv = {{{3},4,{5,6}}, "haha", (u8)45, 46};
/* Compund literal */
struct V gv2 = {(struct S){7,8,{9,10}}, {"hihi", 47}, 48};
/* Parens around compound literal */
struct V gv3 = {((struct S){7,8,{9,10}}), {"hoho", 49}, 50};
/* Initialization of a flex array member (warns in GCC) */
struct W gw = {{1,2,3,4}, {1,2,3,4,5}};

union UU {
    u8 a;
    u8 b;
};
struct SU {
    union UU u;
    u8 c;
};
struct SU gsu = {5,6};

#include <stdio.h>
void print_ (const char *name, const u8 *p, long size)
{
  printf ("%s:", name);
  while (size--) {
      printf (" %x", *p++);
  }
  printf ("\n");
}
#define print(x) print_(#x, (u8*)&x, sizeof (x))
#if 1
void foo (struct W *w)
{
  struct S ls = {1, 2, 3, 4};
  struct S ls2 = {1, 2, {3, 4}};
  struct T lt = {"hello", 42};
  struct U lu = {3, 5,6,7,8, 4, "huhu", 43};
  struct U lu1 = {3, ls, 4, {"huhu", 43}};
  struct U lu2 = {3, (ls), 4, {"huhu", 43}};
  /* Incomplete bracing.  */
  struct U lu21 = {3, ls, 4, "huhu", 43};
  /* Optional braces around scalar initializers.  Accepted, but with
     a warning.  */
  struct U lu3 = { 3, {5,6,7,8,}, 4, {"huhu", 43}};
  /* Many superfluous braces and leaving out one initializer for U.s.c[1] */
  struct U lu4 = { 3, {5,6,7,},  5, { "bla", 44} };
  /* Superfluous braces and useless parens around values */
  struct S ls3 = { (1), (2), {(((3))), 4}};
  /* Superfluous braces, and leaving out braces for V.t, plus cast */
  struct V lv = {{3,4,{5,6}}, "haha", (u8)45, 46};
  /* Compund literal */
  struct V lv2 = {(struct S)w->t.s, {"hihi", 47}, 48};
  /* Parens around compound literal */
  struct V lv3 = {((struct S){7,8,{9,10}}), w->t.t, 50};
  print(ls);
  print(ls2);
  print(lt);
  print(lu);
  print(lu1);
  print(lu2);
  print(lu21);
  print(lu3);
  print(lu4);
  print(ls3);
  print(lv);
  print(lv2);
  print(lv3);
}
#endif

int main()
{
  print(ce);
  print(gs);
  print(gs2);
  print(gt);
  print(gu);
  print(gu2);
  print(gu3);
  print(gu4);
  print(gs3);
  print(gv);
  print(gv2);
  print(gv3);
  print(sinit16);
  print(gw);
  print(gsu);
  foo(&gw);
  //printf("q: %s\n", q);
  return 0;
}
