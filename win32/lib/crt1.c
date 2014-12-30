// =============================================
// crt1.c

// For ExitProcess
// windows.h header file should be included before any other library include
#include <windows.h>
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

// Prototype of __getmainargs:
// in msvcrt v6.x : returns void (windows 98, 2000)
// in msvcrt v7.x : returns int  (windows xp and above)
// Using the last for check the result. A negative value means no success.
// These are the test results of call to __getmainargs with the heap fully:
// win 98, 2000 :
//     program termination with error code 255 after call _amsg_exit(8)
//     prints: "runtime error R6008\r\n- not enough space for arguments\r\n"
// win xp :
//     returns -1
// win 7, 8 :
//     program termination with error code -1 after call ExitProcess(-1)
//
// Checking the return of this function also works on windows 98 and 2000
// because internally it sets to eax the value of the third parameter.
// In this case is &env and at that point it is not a negative value.

int __cdecl __getmainargs(int *pargc, char ***pargv, char ***penv, int globb, _startupinfo*);
void __cdecl __set_app_type(int apptype);
unsigned int __cdecl _controlfp(unsigned int new_value, unsigned int mask);

void _start(void);
int main(int argc, char * argv[], char * env[]);

void _start(void)
{
    __TRY__
    int argc;
    char **argv;
    char **env;
    _startupinfo start_info;

    // Sets the current application type
    __set_app_type(_CONSOLE_APP);

    // Set default FP precision to 53 bits (8-byte double)
    // _MCW_PC (Precision control) is not supported on
    // the ARM and x64 architectures
#if defined(_X86_) && !defined(__x86_64)
    _controlfp(_PC_53, _MCW_PC);
#endif

    start_info.newmode = 0;
    if ( __getmainargs( &argc, &argv, &env, 0, &start_info ) < 0 )
    {
        ExitProcess(-1);
    }
    else
    {
        exit( main(argc, argv, env) );
    }

}

// =============================================
