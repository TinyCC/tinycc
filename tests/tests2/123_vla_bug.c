typedef __SIZE_TYPE__ size_t;
extern int printf(const char*, ...);
extern size_t strlen(const char*);
char str[] = "blabla";
int g;
int main()
{
  //char helpme[strlen(str) + 1];
  int i = 0;
#if 0
  if (g) {
      char buf[strlen(str) + 10];
      buf[0] = 0;
  }
alabel:
  printf("default: i = %d\n", i);
#else
  for (i = 0; i < 5; i++) {
      switch (i) {
      case 10:
          if (g) {
              char buf[strlen(str) + 10];
              buf[0] = 0;
              goto do_cmd;
          }
          break;
      case 1:
          printf("reached 3\n");
    do_cmd:
          printf("after do_cmd");
          break;
      default:
          g++;
          printf("default: i = %d\n", i);
          break;
      }
  }
#endif
  return 0;
}
