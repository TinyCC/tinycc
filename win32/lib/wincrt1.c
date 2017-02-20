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
    _TCHAR *szCmd;
    STARTUPINFO startinfo;
    int fShow;
    int ret;

    __set_app_type(__GUI_APP);
    _controlfp(0x10000, 0x30000);

    szCmd = GetCommandLine();
    if (szCmd) {
        while (__T(' ') == *szCmd)
            szCmd++;
        if (__T('\"') == *szCmd) {
            while (*++szCmd)
                if (__T('\"') == *szCmd) {
                    szCmd++;
                    break;
                }
        } else {
            while (*szCmd && __T(' ') != *szCmd)
                szCmd++;
        }
        while (__T(' ') == *szCmd)
            szCmd++;
    }

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
    int wargc;
    _TCHAR **wargv, **wenv;
    _startupinfo start_info = {0};

    __tgetmainargs(&wargc, &wargv, &wenv, 0, &start_info);
    if (argc < wargc)
        wargv += wargc - argc;
    else
        argc = wargc;
#define argv wargv
#endif

    p = GetCommandLine();
    szCmd = NULL;
    if (argc > 1)
        szCmd = _tcsstr(p, argv[1]);
    if (NULL == szCmd)
        szCmd = __T("");
    else if (szCmd > p && szCmd[-1] == __T('\"'))
        --szCmd;
    _controlfp(0x10000, 0x30000);
    return _tWinMain(GetModuleHandle(NULL), NULL, szCmd, SW_SHOWDEFAULT);
}
