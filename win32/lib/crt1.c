// =============================================
// crt1.c

#include <stdio.h>
#include <stdlib.h>

#define _UNKNOWN_APP    0
#define _CONSOLE_APP    1
#define _GUI_APP        2

#define _MCW_PC         0x00030000 // Precision Control
#define _PC_24          0x00020000 // 24 bits
#define _PC_53          0x00010000 // 53 bits
#define _PC_64          0x00000000 // 64 bits

typedef struct
{
    int newmode;
} _startupinfo;

int __cdecl __getmainargs(int *pargc, char ***pargv, char ***penv, int globb, _startupinfo*);
void __cdecl __set_app_type(int apptype);
unsigned int __cdecl _controlfp(unsigned int new_value, unsigned int mask);
int main(int argc, char * argv[], char * env[]);

/* Allow command-line globbing with "int _dowildcard = 1;" in the user source */
int _dowildcard;

void _start(void)
{
    __TRY__
    int argc, ret;
    char **argv;
    char **env;
    _startupinfo start_info;

    // Sets the current application type
    __set_app_type(_CONSOLE_APP);

    // Set default FP precision to 53 bits (8-byte double)
    // _MCW_PC (Precision control) is not supported on
    // the ARM and x64 architectures
#ifdef __i386
    _controlfp(_PC_53, _MCW_PC);
#endif

    start_info.newmode = 0;
    __getmainargs( &argc, &argv, &env, _dowildcard, &start_info);
    ret = main(argc, argv, env);
    exit(ret);
}

// =============================================
