#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;

/* only correct for i386 */
/* XXX: incorrect for type size different than 4 bytes*/
#define va_start(ap,last) ap = ((char *)&(last)) + sizeof(int); 
#define va_arg(ap,type) (ap += sizeof(int), *(type *)(ap - sizeof(int)))
#define va_end(ap)

/* fix a buggy dependency on GCC in libio.h */
typedef va_list __gnuc_va_list;

#endif
