// =============================================
// crt1.c

#include <stdlib.h>
// For ExitProcess
#include <windows.h>

#define __UNKNOWN_APP    0
#define __CONSOLE_APP    1
#define __GUI_APP        2
void __set_app_type(int);
void _controlfp(unsigned a, unsigned b);

typedef struct
{
    int newmode;
} _startupinfo;

// prototype of __getmainargs:
// in msvcrt v6.0 : return void
// in msvcrt v7.0 : return int
// using first for compatibility
void __getmainargs(int *pargc, char ***pargv, char ***penv, int globb, _startupinfo*);
int main(int argc, char **argv, char **env);

int _start(void)
{
    __TRY__
    int argc; char **argv; char **env; int ret;
    _startupinfo start_info = {0};

    _controlfp(0x10000, 0x30000);
    __set_app_type(__CONSOLE_APP);

    argv = NULL;
    __getmainargs(&argc, &argv, &env, 0, &start_info);
    // check success comparing if argv now is not NULL
    if (! argv)
    {
        ExitProcess(-1);
    }

    ret = main(argc, argv, env);
    exit(ret);
}

// =============================================
