//+---------------------------------------------------------------------------

// _UNICODE for tchar.h, UNICODE for API
#include <tchar.h>

#include <windows.h>
#include <stdlib.h>

#define __UNKNOWN_APP    0
#define __CONSOLE_APP    1
#define __GUI_APP        2
void __set_app_type(int);
void _controlfp(unsigned a, unsigned b);

#ifdef _UNICODE
#define __tgetmainargs __wgetmainargs
#define _twinstart _wwinstart
#define _runtwinmain _runwwinmain
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int);
#else
#define __tgetmainargs __getmainargs
#define _twinstart _winstart
#define _runtwinmain _runwinmain
#endif

typedef struct
{
    int newmode;
} _startupinfo; // CLI Vs GUI

int __cdecl __tgetmainargs(int *pargc, _TCHAR ***pargv, _TCHAR ***penv, int globb, _startupinfo*);

int _twinstart(void)
{
    __TRY__
    _TCHAR *szCmd, *p;
    STARTUPINFO startinfo;
    _startupinfo start_info_con = {0};
    int fShow;
    int ret;

    __set_app_type(__GUI_APP);
    _controlfp(0x10000, 0x30000);

    start_info_con.newmode = 0;
    __tgetmainargs(&__argc, &__targv, &_tenviron, 0, &start_info_con);

    p = GetCommandLine();
    if (__argc > 1)
        szCmd = _tcsstr(p, __targv[1]);
    if (NULL == szCmd)
        szCmd = __T("");
    else if (szCmd > p && szCmd[-1] == __T('\"'))
        --szCmd;

    GetStartupInfo(&startinfo);
    fShow = startinfo.wShowWindow;
    if (0 == (startinfo.dwFlags & STARTF_USESHOWWINDOW))
        fShow = SW_SHOWDEFAULT;

    ret = _tWinMain(GetModuleHandle(NULL), NULL, szCmd, fShow);
    exit(ret);
}

int _runtwinmain(int argc, /* as tcc passed in */ char **argv)
{
    _TCHAR *szCmd, *p;

#ifdef UNICODE
    _startupinfo start_info = {0};

    __tgetmainargs(&__argc, &__targv, &_tenviron, 0, &start_info);
    /* may be wrong when tcc has received wildcards (*.c) */
    if (argc < __argc) {
        __targv += __argc - argc;
        __argc = argc;
    }
#else
    __argc = argc;
    __targv = argv;
#endif

    p = GetCommandLine();
    szCmd = NULL;
    if (argc > 1)
        szCmd = _tcsstr(p, __targv[1]);
    if (NULL == szCmd)
        szCmd = __T("");
    else if (szCmd > p && szCmd[-1] == __T('\"'))
        --szCmd;
    _controlfp(0x10000, 0x30000);
    return _tWinMain(GetModuleHandle(NULL), NULL, szCmd, SW_SHOWDEFAULT);
}
