#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;

/* only correct for i386 */
#define va_start(ap,last) ap=(char *)&(last); 
#define va_arg(ap,type) (ap-=sizeof (type), *(type *)(ap))
#define va_end(ap)

#endif
