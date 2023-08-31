#include <stdio.h>
#include <math.h>

float fd;

int
test()
{
   // was an internal tcc compiler error with arm64 backend until 2019-11-08
   if (fd < 5.5) {
     return 1;
   } else {
     return 0;
   }
}

float f1 = 12.34f, f2 = 56.78f;
double d1 = 12.34, d2 = 56.78;
long double ld1 = 12.34l, ld2 = 56.78l;

float tf = 12.34f + 56.78f - 12.34f * 56.78f + 12.34f / 56.78f;
double td = 12.34 + 56.78 - 12.34 * 56.78 + 12.34 / 56.78;
long double tld = 12.34l + 56.78l - 12.34l * 56.78l + 12.34l / 56.78l;

int main()
{
   static int e1 = -1.0 == 0.0;
   static int e2 = -1.0 != 0.0;
   static int e3 = -1.0 < 0.0;
   static int e4 = -1.0 >= 0.0;
   static int e5 = -1.0 <= 0.0;
   static int e6 = -1.0 > 0.0;
   // variables
   float af = 12.34f + 56.78f;
   double ad = 12.34 + 56.78;
   long double ald = 12.34l + 56.78l;
   printf("%f %f \n", af, tf);
   printf("%g %g\n", ad, td);
   printf("%Lf %Lf\n", ald, tld);

   // infix operators
   printf("%f %f %f %f\n", 12.34f + 56.78f, 12.34f - 56.78f, 12.34f * 56.78f, 12.34f / 56.78f);
   printf("%g %g %g %g\n", 12.34 + 56.78, 12.34 - 56.78, 12.34 * 56.78, 12.34 / 56.78);
   printf("%Lf %Lf %Lf %Lf\n", 12.34l + 56.78l, 12.34l - 56.78l, 12.34l * 56.78l, 12.34l / 56.78l);
#ifdef __i386__
   // gcc/clang -m32 -mno-sse and tcc all fail for + and * but all have the same value
   printf("%f %f %f %f\n", 12.34f + 56.78f, f1 - f2, 12.34f * 56.78f, f1 / f2);
#else
   printf("%f %f %f %f\n", f1 + f2, f1 - f2, f1 * f2, f1 / f2);
#endif
   printf("%g %g %g %g\n", d1 + d2, d1 - d2, d1 * d2, d1 / d2);
   printf("%Lf %Lf %Lf %Lf\n", ld1 + ld2, ld1 - ld2, ld1 * ld2, ld1 / ld2);

   // comparison operators
   printf("%d %d %d %d %d %d\n", 12.34f < 56.78f, 12.34f <= 56.78f, 12.34f == 56.78f, 12.34f >= 56.78f, 12.34f > 56.78f, 12.34f != 56.78f);
   printf("%d %d %d %d %d %d\n", 12.34f < 12.34f, 12.34f <= 12.34f, 12.34f == 12.34f, 12.34f >= 12.34f, 12.34f > 12.34f, 12.34f != 12.34f);
   printf("%d %d %d %d %d %d\n", 56.78f < 12.34f, 56.78f <= 12.34f, 56.78f == 12.34f, 56.78f >= 12.34f, 56.78f > 12.34f, 56.78f != 12.34f);
   printf("%d %d %d %d %d %d\n", 12.34 < 56.78, 12.34 <= 56.78, 12.34 == 56.78, 12.34 >= 56.78, 12.34 > 56.78, 12.34 != 56.78);
   printf("%d %d %d %d %d %d\n", 12.34 < 12.34, 12.34 <= 12.34, 12.34 == 12.34, 12.34 >= 12.34, 12.34 > 12.34, 12.34 != 12.34);
   printf("%d %d %d %d %d %d\n", 56.78 < 12.34, 56.78 <= 12.34, 56.78 == 12.34, 56.78 >= 12.34, 56.78 > 12.34, 56.78 != 12.34);
   printf("%d %d %d %d %d %d\n", 12.34l < 56.78l, 12.34l <= 56.78l, 12.34l == 56.78l, 12.34l >= 56.78l, 12.34l > 56.78l, 12.34l != 56.78l);
   printf("%d %d %d %d %d %d\n", 12.34l < 12.34l, 12.34l <= 12.34l, 12.34l == 12.34l, 12.34l >= 12.34l, 12.34l > 12.34l, 12.34l != 12.34l);
   printf("%d %d %d %d %d %d\n", 56.78l < 12.34l, 56.78l <= 12.34l, 56.78l == 12.34l, 56.78l >= 12.34l, 56.78l > 12.34l, 56.78l != 12.34l);
   printf("%d %d %d %d %d %d\n", f1 < f2, f1 <= f2, f1 == f2, f1 >= f2, f1 > f2, f1 != f2);
   printf("%d %d %d %d %d %d\n", f1 < f1, f1 <= f1, f1 == f1, f1 >= f1, f1 > f1, f1 != f1);
   printf("%d %d %d %d %d %d\n", f2 < f1, f2 <= f1, f2 == f1, f2 >= f1, f2 > f1, f2 != f1);
   printf("%d %d %d %d %d %d\n", d1 < d2, d1 <= d2, d1 == d2, d1 >= d2, d1 > d2, d1 != d2);
   printf("%d %d %d %d %d %d\n", d1 < d1, d1 <= d1, d1 == d1, d1 >= d1, d1 > d1, d1 != d1);
   printf("%d %d %d %d %d %d\n", d2 < d1, d2 <= d1, d2 == d1, d2 >= d1, d2 > d1, d2 != d1);
   printf("%d %d %d %d %d %d\n", ld1 < ld2, ld1 <= ld2, ld1 == ld2, ld1 >= ld2, ld1 > ld2, ld1 != ld2);
   printf("%d %d %d %d %d %d\n", ld1 < ld1, ld1 <= ld1, ld1 == ld1, ld1 >= ld1, ld1 > ld1, ld1 != ld1);
   printf("%d %d %d %d %d %d\n", ld2 < ld1, ld2 <= ld1, ld2 == ld1, ld2 >= ld1, ld2 > ld1, ld2 != ld1);
   printf("%d %d %d %d %d %d\n", e1, e2, e3, e4, e5, e6);

   // assignment operators
   af = 12.34f; ad = 12.34; ald = 12.34l;
   af += 56.78f; ad += 56.78; ald += 56.78l;
   printf("%f %g %Lf\n", af, ad, ald);
#ifdef __i386__
   printf("%f %g %Lf\n", af, ad, ald);
#else
   af = f1; ad = d1; ald = ld1;
   af += f2; ad += d2; ald += ld2;
   printf("%f %g %Lf\n", af, ad, ald);
#endif

   af = 12.34f; ad = 12.34; ald = 12.34l;
   af -= 56.78f; ad -= 56.78; ald -= 56.78l;
   printf("%f %g %Lf\n", af, ad, ald);
   af = f1; ad = d1; ald = ld1;
   af -= f2; ad -= d2; ald -= ld2;
   printf("%f %g %Lf\n", af, ad, ald);

   af = 12.34f; ad = 12.34; ald = 12.34l;
   af *= 56.78f; ad *= 56.78; ald *= 56.78l;
   printf("%f %g %Lf\n", af, ad, ald);
#ifdef __i386__
   printf("%f %g %Lf\n", af, ad, ald);
#else
   af = f1; ad = d1; ald = ld1;
   af *= f2; ad *= d2; ald *= ld2;
   printf("%f %g %Lf\n", af, ad, ald);
#endif

   af = 12.34f; ad = 12.34; ald = 12.34l;
   af /= 56.78f; ad /= 56.78; ald /= 56.78l;
   printf("%f %g %Lf\n", af, ad, ald);
   af = f1; ad = d1; ald = ld1;
   af /= f2; ad /= d2; ald /= ld2;
   printf("%f %g %Lf\n", af, ad, ald);

   // prefix operators
   printf("%f %g %Lf\n", +12.34f, +12.34, +12.34l);
   printf("%f %g %Lf\n", -12.34f, -12.34, -12.34l);

   // type coercion
   af = 2;
   printf("%f\n", af);
   printf("%f\n", sin(2));

   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/
