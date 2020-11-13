#ifndef _TCC_LIBM_H_
#define _TCC_LIBM_H_

#include "../math.h"

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
  union {double f; uint64_t i;} u = {x};
  int e = u.i>>52 & 0x7ff;
  if (!e) return u.i<<1 ? FP_SUBNORMAL : FP_ZERO;
  if (e==0x7ff) return u.i<<12 ? FP_NAN : FP_INFINITE;
  return FP_NORMAL;
}

__CRT_INLINE int __cdecl __fpclassifyf (float x) {
  union {float f; uint32_t i;} u = {x};
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
  union {double d; uint64_t i;} y = { x };
  return y.i>>63;
}

__CRT_INLINE int __cdecl __signbitf (float x) {
  union {float f; uint32_t i; } y = { x };
  return y.i>>31;
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

#define TCCFP_FORCE_EVAL(x) do {            \
if (sizeof(x) == sizeof(float)) {           \
  volatile float __x;                       \
  __x = (x);                                \
} else if (sizeof(x) == sizeof(double)) {   \
  volatile double __x;                      \
  __x = (x);                                \
} else {                                    \
  volatile long double __x;                 \
  __x = (x);                                \
}                                           \
} while(0)

__CRT_INLINE double __cdecl round (double x) {
  union {double f; uint64_t i;} u = {x};
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
  if (y > 0.5)
    y = y + x - 1;
  else if (y <= -0.5)
    y = y + x + 1;
  else
    y = y + x;
  if (u.i >> 63)
    y = -y;
  return y;
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


/*******************************************************************************
  End of code based on MUSL
*******************************************************************************/


__CRT_INLINE double __cdecl cbrt(double x) {
  return (1.0 - ((x < 0.0) << 1)) * pow(fabs(x), 1.0 / 3.0);
}
__CRT_INLINE float __cdecl cbrtf(float x) {
  return (1.0f - ((x < 0.0f) << 1)) * (float) pow(fabs(x), 1.0 / 3.0);
}

__CRT_INLINE double __cdecl log2(double x) {
  return log(x) * 1.4426950408889634073599246810019;
}
__CRT_INLINE float __cdecl log2f(float x) {
  return logf(x) * 1.4426950408889634073599246810019f;
}

__CRT_INLINE double __cdecl exp2(double x) {
  return exp(x * 0.69314718055994530941723212145818);
}
__CRT_INLINE float __cdecl exp2f(float x) {
  return expf(x * 0.69314718055994530941723212145818f);
}

/* tgamma and lgamma: Lanczos approximation
 * https://rosettacode.org/wiki/Gamma_function
 * https://www.johndcook.com/blog/cpp_gamma
 */
__CRT_INLINE double __cdecl lgamma(double x);

__CRT_INLINE double __cdecl tgamma(double x) {
  double m = 1.0, t = 3.14159265358979323;
  if (x == (int)x) {
    for (int k = 2; k < x; ++k) m *= k;
    return x > 0.0 ? m : (x == 0.0 ? INFINITY : NAN);
  }
  if (x < 0.5)
    return t / (sin(t * x) * tgamma(1.0 - x));
  if (x > 12.0)
    return exp(lgamma(x));
  static const double c[8] = {676.5203681218851, -1259.1392167224028,
                              771.32342877765313, -176.61502916214059,
                              12.507343278686905, -0.13857109526572012,
                              9.9843695780195716e-6, 1.5056327351493116e-7};
  m = 0.99999999999980993, t = x + (8 - 1.5);
  for (int k = 0; k < 8; ++k) m += c[k] / (x + k);
  return 2.50662827463100050 * pow(t, x - 0.5) * exp(-t) * m; /* sqrt(2pi) */
}

__CRT_INLINE double __cdecl lgamma(double x) {
  if (x < 12.0)
    return x <= 0.0 && x == (int)x ? INFINITY : log(fabs(tgamma(x)));
  static const double c[7] = {1.0/12.0, -1.0/360.0, 1.0/1260.0, -1.0/1680.0,
                              1.0/1188.0, -691.0/360360.0, 1.0/156.0};
  double m = -3617.0/122400.0, z = 1.0 / (x * x);
  for (int k = 6; k >= 0; --k) m = m * z + c[k];
  return (x - 0.5) * log(x) - x + 0.918938533204672742 + m / x; /* log(2pi)/2 */
}

__CRT_INLINE float __cdecl tgammaf(float x) {
  return tgamma(x);
}

__CRT_INLINE float __cdecl lgammaf(float x) {
  return lgamma(x);
}

#endif /* _TCC_LIBM_H_ */
