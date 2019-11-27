//+---------------------------------------------------------------------------

#include <windows.h>

extern void (*__init_array_start[]) (void);
extern void (*__init_array_end[]) (void);
extern void (*__fini_array_start[]) (void);
extern void (*__fini_array_end[]) (void);

BOOL WINAPI DllMain (HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved);

BOOL WINAPI _dllstart(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL bRet;
    int i;

    if (dwReason == DLL_PROCESS_ATTACH) { /* ignore DLL_THREAD_ATTACH */
        i = 0;
        while (&__init_array_start[i] != __init_array_end) {
            (*__init_array_start[i++])();
        }
    }
    if (dwReason == DLL_PROCESS_DETACH) { /* ignore  DLL_THREAD_DETACH */
        i = 0;
        while (&__fini_array_end[i] != __fini_array_start) {
            (*__fini_array_end[--i])();
        }
    }
    bRet = DllMain (hDll, dwReason, lpReserved);
    return bRet;
}

