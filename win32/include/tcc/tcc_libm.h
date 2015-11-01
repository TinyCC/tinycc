#ifndef _TCC_LIBM_H_
#define _TCC_LIBM_H_

#include "../include/math.h"

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

/*******************************************************************************
  End of code based on MUSL
*******************************************************************************/

#endif /* _TCC_LIBM_H_ */
