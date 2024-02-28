/* ------------------------------------------------------------- */
/* support for tcc_run() */

#ifdef __leading_underscore
# define _(s) s
#else
# define _(s) _##s
#endif

#ifndef _WIN32
extern void (*_(_init_array_start)[]) (int argc, char **argv, char **envp);
extern void (*_(_init_array_end)[]) (int argc, char **argv, char **envp);
static void run_ctors(int argc, char **argv, char **env)
{
    int i = 0;
    while (&_(_init_array_start)[i] != _(_init_array_end))
        (*_(_init_array_start)[i++])(argc, argv, env);
}
#endif

extern void (*_(_fini_array_start)[]) (void);
extern void (*_(_fini_array_end)[]) (void);
static void run_dtors(void)
{
    int i = 0;
    while (&_(_fini_array_end)[i] != _(_fini_array_start))
        (*_(_fini_array_end)[--i])();
}

static void *rt_exitfunc[32];
static void *rt_exitarg[32];
static int __rt_nr_exit;

void __run_on_exit(int ret)
{
    int n = __rt_nr_exit;
    while (n)
	--n, ((void(*)(int,void*))rt_exitfunc[n])(ret, rt_exitarg[n]);
}

int on_exit(void *function, void *arg)
{
    int n = __rt_nr_exit;
    if (n < 32) {
	rt_exitfunc[n] = function;
	rt_exitarg[n] = arg;
        __rt_nr_exit = n + 1;
        return 0;
    }
    return 1;
}

int atexit(void (*function)(void))
{
    return on_exit(function, 0);
}

typedef struct rt_frame {
    void *ip, *fp, *sp;
} rt_frame;

__attribute__((noreturn)) void __rt_exit(rt_frame *, int);

void exit(int code)
{
    rt_frame f;
    run_dtors();
    __run_on_exit(code);
    f.fp = 0;
    f.ip = exit;
    __rt_exit(&f, code);
}

#ifndef _WIN32
int main(int, char**, char**);

int _runmain(int argc, char **argv, char **envp)
{
    int ret;
    run_ctors(argc, argv, envp);
    ret = main(argc, argv, envp);
    run_dtors();
    __run_on_exit(ret);
    return ret;
}
#endif
