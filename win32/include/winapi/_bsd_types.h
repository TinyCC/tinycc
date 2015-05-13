/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _BSDTYPES_DEFINED
#define _BSDTYPES_DEFINED

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
typedef unsigned long	u_long;
#if defined(__GNUC__) || \
    defined(__GNUG__)
__extension__
#endif /* gcc / g++ */
typedef unsigned long long u_int64;

#endif /* _BSDTYPES_DEFINED */

