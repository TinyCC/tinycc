_Alignas(16) int i1;
int _Alignas(16) i2;
void _Alignas(16) *p2;
_Alignas(16) i3;
int _Alignas(double) i4;
int _Alignas(int) i5;
#if 0
/* The following are currently wrongly accepted by TCC but really shouldn't.  */
int _Alignas(int _Alignas(16)) i6; //wrong, 'int _Alignas(16)' is no type-name
typedef int _Alignas(16) int16aligned_t; //wrong, _Alignas invalid on typedef
int16aligned_t i7;
#endif
/* i8 should get an alignment of 16, because unlike _Alignas the
   corresponding attribute _does_ apply to type-name, though not in
   some clang versions.  */
int _Alignas(int __attribute__((aligned(16)))) i8;
extern int printf(const char*, ...);
#ifdef _MSC_VER
#define alignof(x) (int)__alignof(x)
#else
#define alignof(x) (int)__alignof__(x)
#endif
int main()
{
  printf("%d %d %d %d\n",
         alignof(i1) == 16, alignof(i4) == alignof(double),
         alignof(i5) == alignof(int) , alignof(i8) == 16);
  return 0;
}
