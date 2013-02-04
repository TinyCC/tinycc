//+---------------------------------------------------------------------------

#include <windows.h>
#include <stdlib.h>

#define __UNKNOWN_APP    0
#define __CONSOLE_APP    1
#define __GUI_APP        2
void __set_app_type(int);
void _controlfp(unsigned a, unsigned b);

int _winstart(void)
{
    __TRY__
    char *szCmd;
    STARTUPINFO startinfo;
    int fShow;
    int ret;

    __set_app_type(__GUI_APP);
    _controlfp(0x10000, 0x30000);

    szCmd = GetCommandLine();
    if (szCmd) {
        while (' ' == *szCmd)
            szCmd++;
        if ('\"' == *szCmd) {
            while (*++szCmd)
                if ('\"' == *szCmd) {
                    szCmd++;
                    break;
                }
        } else {
            while (*szCmd && ' ' != *szCmd)
                szCmd++;
        }
        while (' ' == *szCmd)
            szCmd++;
    }

    GetStartupInfo(&startinfo);
    fShow = startinfo.wShowWindow;
    if (0 == (startinfo.dwFlags & STARTF_USESHOWWINDOW))
        fShow = SW_SHOWDEFAULT;

    ret = WinMain(GetModuleHandle(NULL), NULL, szCmd, fShow);
    exit(ret);
}

int _runwinmain(int argc, char **argv)
{
    char *szCmd, *p;

    p = GetCommandLine();
    szCmd = NULL;
    if (argc > 1)
        szCmd = strstr(p, argv[1]);
    if (NULL == szCmd)
        szCmd = "";
    else if (szCmd > p && szCmd[-1] == '\"')
        --szCmd;
    return WinMain(GetModuleHandle(NULL), NULL, szCmd, SW_SHOWDEFAULT);
}

