/*
 * _mingw.h
 *
 *  This file is for TinyCC and not part of the Mingw32 package.
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __MINGW_H
#define __MINGW_H

/* some winapi files define these before including _mingw.h --> */
#undef __cdecl
#undef _X86_
#undef WIN32
/* <-- */

#include <stddef.h>
#include <stdarg.h>

#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long

#define __cdecl __attribute__((__cdecl__))
#define __declspec(x) __attribute__((x))
#define __unaligned __attribute__((packed))
#define __fastcall __attribute__((fastcall))

#define __MSVCRT__ 1
#undef _MSVCRT_
#define __MINGW_IMPORT extern __declspec(dllimport)
#define __MINGW_ATTRIB_NORETURN
#define __MINGW_ATTRIB_CONST
#define __MINGW_ATTRIB_DEPRECATED
#define __MINGW_ATTRIB_MALLOC
#define __MINGW_ATTRIB_PURE
#define __MINGW_ATTRIB_NONNULL(arg)
#define __MINGW_NOTHROW
#define __GNUC_VA_LIST

#define _CRTIMP extern
#define __CRT_INLINE extern __inline__

#define _CRT_ALIGN(x) __attribute__((aligned(x)))
#define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))
#define _CRT_PACKING 8
#define __CRT_UNALIGNED
#define _CONST_RETURN

#define __CRT_STRINGIZE(_Value) #_Value
#define _CRT_STRINGIZE(_Value) __CRT_STRINGIZE(_Value)
#define __CRT_WIDE(_String) L ## _String
#define _CRT_WIDE(_String) __CRT_WIDE(_String)

#ifdef _WIN64
#define __stdcall
#define _AMD64_ 1
#define __x86_64 1
#define USE_MINGW_SETJMP_TWO_ARGS
#define mingw_getsp tinyc_getbp
#define __TRY__
#else
#define __stdcall __attribute__((__stdcall__))
#define _X86_ 1
#define WIN32 1
#define _USE_32BIT_TIME_T
#define __TRY__ void __try__(void**), *_sehrec[6]; __try__(_sehrec);
#endif

/* in stddef.h */
#define _SIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#define _PTRDIFF_T_DEFINED
#define _WCHAR_T_DEFINED
#define _UINTPTR_T_DEFINED
#define _INTPTR_T_DEFINED

#define _INTEGRAL_MAX_BITS 64

typedef long __time32_t;
#define _TIME32_T_DEFINED
typedef __int64 __time64_t;
#define _TIME64_T_DEFINED
#ifdef _USE_32BIT_TIME_T
typedef __time32_t time_t;
#define _TIME_T_DEFINED
#else
typedef __time64_t time_t;
#define _TIME_T_DEFINED
#endif

typedef unsigned long size_t;
#define _SIZE_T_DEFINED
typedef long ssize_t;
#define _SSIZE_T_DEFINED

typedef unsigned int wint_t;
typedef unsigned short wctype_t;
#define _WCTYPE_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED

typedef int errno_t;
#define _ERRCODE_DEFINED

typedef struct threadlocaleinfostruct *pthreadlocinfo;
typedef struct threadmbcinfostruct *pthreadmbcinfo;
typedef struct localeinfo_struct _locale_tstruct,*_locale_t;

/* for winapi */
#define _ANONYMOUS_UNION
#define _ANONYMOUS_STRUCT
#define DECLSPEC_NORETURN
#define DECLARE_STDCALL_P(type) __stdcall type
#define NOSERVICE 1
#define NOMCX 1
#define NOIME 1
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef WINVER
# define WINVER 0x0502
#endif
#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x502
#endif

#endif /* __MINGW_H */
