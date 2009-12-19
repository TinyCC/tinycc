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

#include <stddef.h>

#define __int64 long long
#define __int32 long
#define __int16 short
#define __int8 char

#define __cdecl __attribute__((__cdecl__))
#define __declspec(x) __attribute__((x))
#define __fastcall

// #define __MINGW_IMPORT extern __declspec(dllimport)

#undef _MSVCRT_
#undef __MINGW_IMPORT

#define _CRTIMP extern
#define __CRT_INLINE extern __inline__
#define __MINGW_NOTHROW
#define __MINGW_ATTRIB_NORETURN
#define __MINGW_ATTRIB_DEPRECATED
#define __GNUC_VA_LIST

#define _CONST_RETURN
#define _CRT_ALIGN(x) __attribute__((aligned(x)))
#define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))

#define __CRT_STRINGIZE(_Value) #_Value
#define _CRT_STRINGIZE(_Value) __CRT_STRINGIZE(_Value)

#define __CRT_WIDE(_String) L ## _String
#define _CRT_WIDE(_String) __CRT_WIDE(_String)

#ifdef _WIN64
typedef __int64 intptr_t;
typedef unsigned __int64 uintptr_t;
#define __stdcall
#define _AMD64_ 1
#define __x86_64 1
#define USE_MINGW_SETJMP_TWO_ARGS
#define mingw_getsp tinyc_getbp
#else
typedef __int32 intptr_t;
typedef unsigned __int32 uintptr_t;
#define __stdcall __attribute__((__stdcall__))
#define _X86_ 1
#define WIN32 1
#endif

#define _INTEGRAL_MAX_BITS 64
#define _CRT_PACKING 8

typedef long __time32_t;
#define _TIME32_T_DEFINED
typedef __int64 __time64_t;
#define _TIME64_T_DEFINED

#ifdef _USE_32BIT_TIME_T
#ifdef _WIN64
#error You cannot use 32-bit time_t (_USE_32BIT_TIME_T) with _WIN64
#endif
typedef __time32_t time_t;
#else
typedef __time64_t time_t;
#endif
#define _TIME_T_DEFINED

#define _WCTYPE_T_DEFINED
typedef unsigned int wint_t;
typedef unsigned short wctype_t;

#define _ERRCODE_DEFINED
typedef int errno_t;

typedef struct threadlocaleinfostruct *pthreadlocinfo;
typedef struct threadmbcinfostruct *pthreadmbcinfo;
typedef struct localeinfo_struct _locale_tstruct,*_locale_t;

/* for winapi */
#define _ANONYMOUS_UNION
#define _ANONYMOUS_STRUCT
#define DECLSPEC_NORETURN
#define WIN32_LEAN_AND_MEAN
#define DECLARE_STDCALL_P(type) __stdcall type
#define NOSERVICE 1
#define NOMCX 1
#define NOIME 1
#ifndef WINVER
#define WINVER 0x0502
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x502
#endif

/* get va_list */
#include <stdarg.h>

#endif /* __MINGW_H */
