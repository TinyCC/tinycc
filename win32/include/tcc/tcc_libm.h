#ifndef _TCC_LIBM_H_
#define _TCC_LIBM_H_

#include "../math.h"
#include "../stdint.h"

/* TCC uses 8 bytes for double and long double, so effectively the l variants
 * are never used. For now, they just run the normal (double) variant.
 */

/*
 * most of the code in this file is taken from MUSL rs-1.0 (MIT license)
 * - musl-libc: http://git.musl-libc.org/cgit/musl/tree/src/math?h=rs-1.0
 * - License:   http://git.musl-libc.org/cgit/musl/tree/COPYRIGHT?h=rs-1.0
 */

/*******************************************************************************
  Start of code based on MUSL
*******************************************************************************/
/*
musl as a whole is licensed under the following standard MIT license:

----------------------------------------------------------------------
Copyright © 2005-2014 Rich Felker, et al.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
----------------------------------------------------------------------
*/

/* fpclassify */

__CRT_INLINE int __cdecl __fpclassify (double x) {
  union {double f; uint64_t i;} u = {.f = x};
  int e = u.i>>52 & 0x7ff;
  if (!e) return u.i<<1 ? FP_SUBNORMAL : FP_ZERO;
  if (e==0x7ff) return u.i<<12 ? FP_NAN : FP_INFINITE;
  return FP_NORMAL;
}

__CRT_INLINE int __cdecl __fpclassifyf (float x) {
  union {float f; uint32_t i;} u = {.f = x};
  int e = u.i>>23 & 0xff;
  if (!e) return u.i<<1 ? FP_SUBNORMAL : FP_ZERO;
  if (e==0xff) return u.i<<9 ? FP_NAN : FP_INFINITE;
  return FP_NORMAL;
}

__CRT_INLINE int __cdecl __fpclassifyl (long double x) {
  return __fpclassify(x);
}


/* signbit */

__CRT_INLINE int __cdecl __signbit (double x) {
  union {double f; uint64_t i;} u = {.f = x};
  return u.i>>63;
}

__CRT_INLINE int __cdecl __signbitf (float x) {
  union {float f; uint32_t i; } u = {.f = x};
  return u.i>>31;
}

__CRT_INLINE int __cdecl __signbitl (long double x) {
  return __signbit(x);
}


/* fmin*, fmax* */

#define TCCFP_FMIN_EVAL (isnan(x) ? y :                                      \
                         isnan(y) ? x :                                      \
                         (signbit(x) != signbit(y)) ? (signbit(x) ? x : y) : \
                         x < y ? x : y)

__CRT_INLINE double __cdecl fmin (double x, double y) {
  return TCCFP_FMIN_EVAL;
}

__CRT_INLINE float __cdecl fminf (float x, float y) {
  return TCCFP_FMIN_EVAL;
}

__CRT_INLINE long double __cdecl fminl (long double x, long double y) {
  return TCCFP_FMIN_EVAL;
}

#define TCCFP_FMAX_EVAL (isnan(x) ? y :                                      \
                         isnan(y) ? x :                                      \
                         (signbit(x) != signbit(y)) ? (signbit(x) ? y : x) : \
                         x < y ? y : x)

__CRT_INLINE double __cdecl fmax (double x, double y) {
  return TCCFP_FMAX_EVAL;
}

__CRT_INLINE float __cdecl fmaxf (float x, float y) {
  return TCCFP_FMAX_EVAL;
}

__CRT_INLINE long double __cdecl fmaxl (long double x, long double y) {
  return TCCFP_FMAX_EVAL;
}


/* *round* */

#define TCCFP_FORCE_EVAL(x) do {  \
  volatile typeof(x) __x;         \
  __x = (x);                      \
} while(0)

__CRT_INLINE double __cdecl round (double x) {
  union {double f; uint64_t i;} u = {.f = x};
  int e = u.i >> 52 & 0x7ff;
  double y;

  if (e >= 0x3ff+52)
    return x;
  if (u.i >> 63)
    x = -x;
  if (e < 0x3ff-1) {
    /* raise inexact if x!=0 */
    TCCFP_FORCE_EVAL(x + 0x1p52);
    return 0*u.f;
  }
  y = (double)(x + 0x1p52) - 0x1p52 - x;
  y = y + x - (y > 0.5) + (y <= -0.5); /* branchless */
  return (u.i >> 63) ? -y : y;
}

__CRT_INLINE long __cdecl lround (double x) {
  return round(x);
}

__CRT_INLINE long long __cdecl llround (double x) {
  return round(x);
}

__CRT_INLINE float __cdecl roundf (float x) {
  return round(x);
}

__CRT_INLINE long __cdecl lroundf (float x) {
  return round(x);
}

__CRT_INLINE long long __cdecl llroundf (float x) {
  return round(x);
}

__CRT_INLINE long double __cdecl roundl (long double x) {
  return round(x);
}

__CRT_INLINE long __cdecl lroundl (long double x) {
  return round(x);
}

__CRT_INLINE long long __cdecl llroundl (long double x) {
  return round(x);
}


/* MUSL asinh, acosh, atanh */

__CRT_INLINE double __cdecl asinh(double x) {
  union {double f; uint64_t i;} u = {.f = x};
  unsigned e = u.i >> 52 & 0x7ff, s = u.i >> 63;
  u.i &= -1ull / 2, x = u.f;
  if (e >= 0x3ff + 26) x = log(x) + 0.693147180559945309;
  else if (e >= 0x3ff + 1) x = log(2*x + 1 / (sqrt(x*x + 1) + x)); /* |x|>=2 */
  else if (e >= 0x3ff - 26) x = log1p(x + x*x / (sqrt(x*x + 1) + 1));
  else TCCFP_FORCE_EVAL(x + 0x1p120f);
  return s ? -x : x;
}

__CRT_INLINE double __cdecl acosh(double x) {
  union {double f; uint64_t i;} u = {.f = x};
  unsigned e = u.i >> 52 & 0x7ff;
  if (e < 0x3ff + 1) return --x, log1p(x + sqrt(x*x + 2*x)); /* |x|<2 */
  if (e < 0x3ff + 26) return log(2*x - 1 / (x + sqrt(x*x - 1)));
  return log(x) + 0.693147180559945309;
}

__CRT_INLINE double __cdecl atanh(double x) {
  union {double f; uint64_t i;} u = {.f = x};
  unsigned e = u.i >> 52 & 0x7ff, s = u.i >> 63;
  u.i &= -1ull / 2, x = u.f;
  if (e < 0x3ff - 1) {
    if (e < 0x3ff - 32) { if (e == 0) TCCFP_FORCE_EVAL((float)x); }
    else x = 0.5 * log1p(2*x + 2*x*x / (1 - x)); /* |x| < 0.5 */
  } else x = 0.5 * log1p(2*(x / (1 - x))); /* avoid overflow */
  return s ? -x : x;
}

/* MUSL scalbn */

__CRT_INLINE double __cdecl scalbn(double x, int n) {
  union {double f; uint64_t i;} u;
  if (n > 1023) {
    x *= 0x1p1023, n -= 1023;
    if (n > 1023) {
      x *= 0x1p1023, n -= 1023;
      if (n > 1023) n = 1023;
    }
  } else if (n < -1022) {
    x *= 0x1p-1022 * 0x1p53, n += 1022 - 53;
    if (n < -1022) {
      x *= 0x1p-1022 * 0x1p53, n += 1022 - 53;
      if (n < -1022) n = -1022;
    }
  }
  u.i = (0x3ffull + n) << 52;
  return x * u.f;
}

/* MUSL: Override msvcrt frexp(): 4.5x speedup! */

__CRT_INLINE double __cdecl frexp(double x, int *e) {
  union {double f; uint64_t i;} u = {.f = x};
  int ee = u.i>>52 & 0x7ff;
  if (!ee) {
    if (x) x = frexp(x*0x1p64, e), *e -= 64;
    else *e = 0;
    return x;
  } else if (ee == 0x7ff)
    return x;
  *e = ee - 0x3fe;
  u.i &= 0x800fffffffffffffull;
  u.i |= 0x3fe0000000000000ull;
  return u.f;
}

/* MUSL nan */

__CRT_INLINE double __cdecl nan(const char* s) {
  return NAN;
}
__CRT_INLINE float __cdecl nanf(const char* s) {
  return NAN;
}
__CRT_INLINE long double __cdecl nanl(const char* s) {
  return NAN;
}


/*******************************************************************************
  End of code based on MUSL
*******************************************************************************/


/* Following are math functions missing from msvcrt.dll, and not defined
 * in math.h or above. Functions still remaining:
 * remquo(), remainder(), fma(), erf(), erfc(), nearbyint().
 * In <stdlib.h>: lldiv().
 */

__CRT_INLINE float __cdecl scalbnf(float x, int n) {
  return scalbn(x, n);
}
__CRT_INLINE long double __cdecl scalbnl(long double x, int n) {
  return scalbn(x, n);
}

__CRT_INLINE double __cdecl scalbln(double x, long n) {
  return scalbn(x, n);
}
__CRT_INLINE float __cdecl scalblnf(float x, long n) {
  return scalbn(x, n);
}
__CRT_INLINE long double __cdecl scalblnl(long double x, long n) {
  return scalbn(x, n);
}

/* Override msvcrt ldexp(): 7.3x speedup! */

__CRT_INLINE double __cdecl ldexp(double x, int expn) {
  return scalbn(x, expn);
}
__CRT_INLINE float __cdecl ldexpf(float x, int expn) {
  return scalbn(x, expn);
}
__CRT_INLINE long double __cdecl ldexpl(long double x, int expn) {
  return scalbn(x, expn);
}

__CRT_INLINE float __cdecl frexpf(float x, int *y) {
  return frexp(x, y);
}
__CRT_INLINE long double __cdecl frexpl (long double x, int* y) {
  return frexp(x, y);
}


__CRT_INLINE double __cdecl rint(double x) {
double retval;
  __asm__ (
    "fldl    %1\n"
    "frndint   \n"
    "fstpl    %0\n" : "=m" (retval) : "m" (x));
  return retval;
}

__CRT_INLINE float __cdecl rintf(float x) {
  float retval;
  __asm__ (
    "flds    %1\n"
    "frndint   \n"
    "fstps    %0\n" : "=m" (retval) : "m" (x));
   return retval;
}
__CRT_INLINE long double __cdecl rintl (long double x) {
  return rint(x);
}


/* 7.12.9.5 */
__CRT_INLINE long __cdecl lrint(double x) {
  long retval;
  __asm__ __volatile__
    ("fldl   %1\n"
     "fistpl %0"  : "=m" (retval) : "m" (x));
  return retval;
}

__CRT_INLINE long __cdecl lrintf(float x) {
  long retval;
  __asm__ __volatile__
    ("flds   %1\n"
     "fistpl %0"  : "=m" (retval) : "m" (x));
  return retval;
}

__CRT_INLINE long __cdecl lrintl (long double x) {
  return lrint(x);
}


__CRT_INLINE long long __cdecl llrint(double x) {
long long retval;
  __asm__ __volatile__
    ("fldl    %1\n"
     "fistpll %0"  : "=m" (retval) : "m" (x));
  return retval;
}

__CRT_INLINE long long __cdecl llrintf(float x) {
  long long retval;
  __asm__ __volatile__
    ("flds   %1\n"
     "fistpll %0"  : "=m" (retval) : "m" (x));
  return retval;
}

__CRT_INLINE long long __cdecl llrintl (long double x) {
  return llrint(x);
}


__CRT_INLINE double __cdecl trunc(double _x) {
  double retval;
  unsigned short saved_cw;
  unsigned short tmp_cw;
  __asm__ ("fnstcw %0;" : "=m" (saved_cw)); /* save FPU control word */
  tmp_cw = (saved_cw & ~(FE_TONEAREST | FE_DOWNWARD | FE_UPWARD | FE_TOWARDZERO))
    | FE_TOWARDZERO;
  __asm__ ("fldcw %0;" : : "m" (tmp_cw));
  __asm__ ("fldl  %1;"
           "frndint;"
           "fstpl  %0;" : "=m" (retval)  : "m" (_x)); /* round towards zero */
  __asm__ ("fldcw %0;" : : "m" (saved_cw) ); /* restore saved control word */
  return retval;
}

__CRT_INLINE float __cdecl truncf(float x) {
  return (float) ((intptr_t) x);
}
__CRT_INLINE long double __cdecl truncl(long double x) {
  return trunc(x);
}


__CRT_INLINE long double __cdecl nextafterl(long double x, long double to) {
  return nextafter(x, to);
}

__CRT_INLINE double __cdecl nexttoward(double x, long double to) {
  return nextafter(x, to);
}
__CRT_INLINE float __cdecl nexttowardf(float x, long double to) {
  return nextafterf(x, to);
}
__CRT_INLINE long double __cdecl nexttowardl(long double x, long double to) {
  return nextafter(x, to);
}

/* Override msvcrt fabs(): 6.3x speedup! */

__CRT_INLINE double __cdecl fabs(double x) {
  return x < 0 ? -x : x;
}
__CRT_INLINE float __cdecl fabsf(float x) {
  return x < 0 ? -x : x;
}
__CRT_INLINE long double __cdecl fabsl(long double x) {
  return x < 0 ? -x : x;
}


#if defined(_WIN32) && !defined(_WIN64) && !defined(__ia64__)
  __CRT_INLINE float acosf(float x) { return acos(x); }
  __CRT_INLINE float asinf(float x) { return asin(x); }
  __CRT_INLINE float atanf(float x) { return atan(x); }
  __CRT_INLINE float atan2f(float x, float y) { return atan2(x, y); }
  __CRT_INLINE float ceilf(float x) { return ceil(x); }
  __CRT_INLINE float cosf(float x) { return cos(x); }
  __CRT_INLINE float coshf(float x) { return cosh(x); }
  __CRT_INLINE float expf(float x) { return exp(x); }
  __CRT_INLINE float floorf(float x) { return floor(x); }
  __CRT_INLINE float fmodf(float x, float y) { return fmod(x, y); }
  __CRT_INLINE float logf(float x) { return log(x); }
  __CRT_INLINE float logbf(float x) { return logb(x); }
  __CRT_INLINE float log10f(float x) { return log10(x); }
  __CRT_INLINE float modff(float x, float *y) { double di, df = modf(x, &di); *y = di; return df; }
  __CRT_INLINE float powf(float x, float y) { return pow(x, y); }
  __CRT_INLINE float sinf(float x) { return sin(x); }
  __CRT_INLINE float sinhf(float x) { return sinh(x); }
  __CRT_INLINE float sqrtf(float x) { return sqrt(x); }
  __CRT_INLINE float tanf(float x) { return tan(x); }
  __CRT_INLINE float tanhf(float x) { return tanh(x); }
#endif
__CRT_INLINE float __cdecl asinhf(float x) { return asinh(x); }
__CRT_INLINE float __cdecl acoshf(float x) { return acosh(x); }
__CRT_INLINE float __cdecl atanhf(float x) { return atanh(x); }

__CRT_INLINE long double __cdecl asinhl(long double x) { return asinh(x); }
__CRT_INLINE long double __cdecl acoshl(long double x) { return acosh(x); }
__CRT_INLINE long double __cdecl atanhl(long double x) { return atanh(x); }
__CRT_INLINE long double __cdecl asinl(long double x) { return asin(x); }
__CRT_INLINE long double __cdecl acosl(long double x) { return acos(x); }
__CRT_INLINE long double __cdecl atanl(long double x) { return atan(x); }
__CRT_INLINE long double __cdecl ceill(long double x) { return ceil(x); }
__CRT_INLINE long double __cdecl coshl(long double x) { return cosh(x); }
__CRT_INLINE long double __cdecl cosl(long double x) { return cos(x); }
__CRT_INLINE long double __cdecl expl(long double x) { return exp(x); }
__CRT_INLINE long double __cdecl floorl(long double x) { return floor(x); }
__CRT_INLINE long double __cdecl fmodl(long double x, long double y) { return fmod(x, y); }
__CRT_INLINE long double __cdecl hypotl(long double x, long double y) { return hypot(x, y); }
__CRT_INLINE long double __cdecl logl(long double x) { return log(x); }
__CRT_INLINE long double __cdecl logbl(long double x) { return logb(x); }
__CRT_INLINE long double __cdecl log10l(long double x) { return log10(x); }
__CRT_INLINE long double __cdecl modfl(long double x, long double* y) { double y1 = *y; x = modf(x, &y1); *y = y1; return x; }
__CRT_INLINE long double __cdecl powl(long double x, long double y) { return pow(x, y); }
__CRT_INLINE long double __cdecl sinhl(long double x) { return sinh(x); }
__CRT_INLINE long double __cdecl sinl(long double x) { return sin(x); }
__CRT_INLINE long double __cdecl sqrtl(long double x) { return sqrt(x); }
__CRT_INLINE long double __cdecl tanhl(long double x) { return tanh(x); }
__CRT_INLINE long double __cdecl tanl(long double x) { return tan(x); }

/* Following are accurate, but much shorter implementations than MUSL lib. */

__CRT_INLINE double __cdecl log1p(double x) {
  double u = 1.0 + x;
  return u == 1.0 ? x : log(u)*(x / (u - 1.0));
}
__CRT_INLINE float __cdecl log1pf(float x) {
  float u = 1.0f + x;
  return u == 1.0f ? x : logf(u)*(x / (u - 1.0f));
}
__CRT_INLINE long double __cdecl log1pl(long double x) {
  return log1p(x);
}


__CRT_INLINE double __cdecl expm1(double x) {
  if (x > 0.0024 || x < -0.0024) return exp(x) - 1.0;
  return x*(1.0 + 0.5*x*(1.0 + (1/3.0)*x*(1.0 + 0.25*x*(1.0 + 0.2*x))));
}
__CRT_INLINE float __cdecl expm1f(float x) {
  if (x > 0.085f || x < -0.085f) return expf(x) - 1.0f;
  return x*(1.0f + 0.5f*x*(1.0f + (1/3.0f)*x*(1.0f + 0.25f*x)));
}
__CRT_INLINE long double __cdecl expm1l(long double x) {
  return expm1(x);
}


__CRT_INLINE double __cdecl cbrt(double x) {
  return x < 0.0 ? -pow(-x, 1/3.0) : pow(x, 1/3.0);
}
__CRT_INLINE float __cdecl cbrtf(float x) {
  return x < 0.0f ? -pow(-x, 1/3.0) : pow(x, 1/3.0);
}
__CRT_INLINE long double __cdecl cbrtl(long double x) {
  return cbrt(x);
}


__CRT_INLINE double __cdecl log2(double x) {
  return log(x) * 1.442695040888963407;
}
__CRT_INLINE float __cdecl log2f(float x) {
  return log(x) * 1.442695040888963407;
}
__CRT_INLINE long double __cdecl log2l(long double x) {
  return log(x) * 1.442695040888963407;
}


__CRT_INLINE double __cdecl exp2(double x) {
  return exp(x * 0.693147180559945309);
}
__CRT_INLINE float __cdecl exp2f(float x) {
  return exp(x * 0.693147180559945309);
}
__CRT_INLINE long double __cdecl exp2l(long double x) {
  return exp(x * 0.693147180559945309);
}


__CRT_INLINE int __cdecl ilogb(double x) {
  return (int) logb(x);
}
__CRT_INLINE int __cdecl ilogbf(float x) {
  return (int) logbf(x);
}
__CRT_INLINE int __cdecl ilogbl(long double x) {
  return (int) logb(x);
}


__CRT_INLINE double __cdecl fdim(double x, double y) {
  if (isnan(x) || isnan(y)) return NAN;
  return x > y ? x - y : 0;
}
__CRT_INLINE float __cdecl fdimf(float x, float y) {
  if (isnan(x) || isnan(y)) return NAN;
  return x > y ? x - y : 0;
}
__CRT_INLINE long double __cdecl fdiml(long double x, long double y) {
  if (isnan(x) || isnan(y)) return NAN;
  return x > y ? x - y : 0;
}


/* tgamma and lgamma: Lanczos approximation
 * https://rosettacode.org/wiki/Gamma_function
 * https://www.johndcook.com/blog/cpp_gamma
 */

__CRT_INLINE double __cdecl tgamma(double x) {
  double m = 1.0, t = 3.14159265358979323;
  if (x == floor(x)) {
    if (x == 0) return INFINITY;
    if (x < 0) return NAN;
    if (x < 26) { for (double k = 2; k < x; ++k) m *= k; return m; }
  }
  if (x < 0.5)
    return t / (sin(t*x)*tgamma(1.0 - x));
  if (x > 12.0)
    return exp(lgamma(x));

  static const double c[8] = {676.5203681218851, -1259.1392167224028,
                              771.32342877765313, -176.61502916214059,
                              12.507343278686905, -0.13857109526572012,
                              9.9843695780195716e-6, 1.5056327351493116e-7};
  m = 0.99999999999980993, t = x + 6.5; /* x-1+8-.5 */
  for (int k = 0; k < 8; ++k) m += c[k] / (x + k);
  return 2.50662827463100050 * pow(t, x - 0.5)*exp(-t)*m; /* C=sqrt(2pi) */
}


__CRT_INLINE double __cdecl lgamma(double x) {
  if (x < 12.0) {
    if (x <= 0.0 && x == floor(x)) return INFINITY;
    x = tgamma(x);
    return log(x < 0.0 ? -x : x);
  }
  static const double c[7] = {1.0/12.0, -1.0/360.0, 1.0/1260.0, -1.0/1680.0,
                              1.0/1188.0, -691.0/360360.0, 1.0/156.0};
  double m = -3617.0/122400.0, t = 1.0 / (x*x);
  for (int k = 6; k >= 0; --k) m = m*t + c[k];
  return (x - 0.5)*log(x) - x + 0.918938533204672742 + m / x; /* C=log(2pi)/2 */
}

__CRT_INLINE float __cdecl tgammaf(float x) {
  return tgamma(x);
}
__CRT_INLINE float __cdecl lgammaf(float x) {
  return lgamma(x);
}
__CRT_INLINE long double __cdecl tgammal(long double x) {
  return tgamma(x);
}
__CRT_INLINE long double __cdecl lgammal(long double x) {
  return lgamma(x);
}

#endif /* _TCC_LIBM_H_ */
