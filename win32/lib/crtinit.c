//+---------------------------------------------------------------------------

#ifdef __leading_underscore
# define _(s) s
#else
# define _(s) _##s
#endif

extern void (*_(_init_array_start)[]) (int argc, _TCHAR **argv, _TCHAR **envp);
extern void (*_(_init_array_end)[]) (int argc, _TCHAR **argv, _TCHAR **envp);
extern void (*_(_fini_array_start)[]) (void);
extern void (*_(_fini_array_end)[]) (void);

static void run_ctors(int argc, _TCHAR **argv, _TCHAR **env)
{
    int i = 0;
    while (&_(_init_array_start)[i] != _(_init_array_end))
        (*_(_init_array_start)[i++])(argc, argv, env);
}

static void run_dtors(void)
{
    int i = 0;
    while (&_(_fini_array_end)[i] != _(_fini_array_start))
        (*_(_fini_array_end)[--i])();
}
