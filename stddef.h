#ifndef _STDDEF_H
#define _STDDEF_H

#define NULL ((void *)0)
typedef unsigned int size_t;
typedef int wchar_t;
typedef int ptrdiff_t;
#define offsetof(type, field) ((size_t) &((type *)0)->field)

#endif
