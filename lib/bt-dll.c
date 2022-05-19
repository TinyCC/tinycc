/* ------------------------------------------------------------- */
/* stubs for calling bcheck functions from a dll. */

#include <windows.h>
#include <stdio.h>

#define REDIR_ALL \
  REDIR(__bt_init) \
  REDIR(__bt_exit) \
  REDIR(tcc_backtrace) \
  \
  REDIR(__bound_ptr_add) \
  REDIR(__bound_ptr_indir1) \
  REDIR(__bound_ptr_indir2) \
  REDIR(__bound_ptr_indir4) \
  REDIR(__bound_ptr_indir8) \
  REDIR(__bound_ptr_indir12) \
  REDIR(__bound_ptr_indir16) \
  REDIR(__bound_local_new) \
  REDIR(__bound_local_delete) \
  REDIR(__bound_new_region) \
  \
  REDIR(__bound_free) \
  REDIR(__bound_malloc) \
  REDIR(__bound_realloc) \
  REDIR(__bound_memcpy) \
  REDIR(__bound_memcmp) \
  REDIR(__bound_memmove) \
  REDIR(__bound_memset) \
  REDIR(__bound_strlen) \
  REDIR(__bound_strcpy) \
  REDIR(__bound_strncpy) \
  REDIR(__bound_strcmp) \
  REDIR(__bound_strncmp) \
  REDIR(__bound_strcat) \
  REDIR(__bound_strchr) \
  REDIR(__bound_strdup)

#ifdef __leading_underscore
#define _(s) "_"#s
#else
#define _(s) #s
#endif

#define REDIR(s) void *s;
static struct { REDIR_ALL } all_ptrs;
#undef REDIR
#define REDIR(s) #s"\0"
static const char all_names[] = REDIR_ALL;
#undef REDIR
#define REDIR(s) __asm__(".global " _(s) ";" _(s) ": jmp *%0" : : "m" (all_ptrs.s) );
static void all_jmps() { REDIR_ALL }
#undef REDIR

void __bt_init_dll(int bcheck)
{
    const char *s = all_names;
    void **p = (void**)&all_ptrs;
    do {
        *p = (void*)GetProcAddress(GetModuleHandle(NULL), (char*)s);
        if (NULL == *p) {
            char buf[100];
            sprintf(buf,
                "Error: function '%s()' not found in executable. "
                "(Need -bt or -b for linking the exe.)", s);
            if (GetStdHandle(STD_ERROR_HANDLE))
                fprintf(stderr, "TCC/BCHECK: %s\n", buf), fflush(stderr);
            else
                MessageBox(NULL, buf, "TCC/BCHECK", MB_ICONERROR);
            ExitProcess(1);
        }
        s = strchr(s,'\0') + 1, ++p;
    } while (*s && (bcheck || p < &all_ptrs.__bound_ptr_add));
}
