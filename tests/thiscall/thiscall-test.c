void ( __attribute__((thiscall)) * const thisCall1 ) ( void * _this, int a ) = ( void ( __attribute__((thiscall)) * ) ( void * _this, int a ) ) 0x4788a0;


int main() {
  thisCall1((void*) 0xDEADBEEF, 1000);

  return 1;
}