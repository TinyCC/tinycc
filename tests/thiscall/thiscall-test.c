#ifndef __thiscall
  #define __thiscall __attribute__((thiscall))
#endif
#ifndef __cdecl
  #define __cdecl __attribute__((cdecl))  
#endif
#ifndef __stdcall
  #define __stdcall __attribute__((stdcall))  
#endif

void ( __thiscall * const thisCall1 ) ( void * _this, int a ) = ( void ( __thiscall * ) ( void * _this, int a ) ) 0x4788a0;

int main() {
  thisCall1((void*) 0xDEADBEEF, 1000);

  return 1;
}