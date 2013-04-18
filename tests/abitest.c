#include <libtcc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *tccdir = NULL;

typedef int (*callback_type) (void*);

/*
 * Compile source code and call a callback with a pointer to the symbol "f".
 */
static int run_callback(const char *src, callback_type callback) {
  TCCState *s;
  int result;
  void *ptr;
  
  s = tcc_new();
  if (!s)
    return -1;
  if (tccdir)
    tcc_set_lib_path(s, tccdir);
  if (tcc_set_output_type(s, TCC_OUTPUT_MEMORY) == -1)
    return -1;
  if (tcc_compile_string(s, src) == -1)
    return -1;
  if (tcc_relocate(s, TCC_RELOCATE_AUTO) == -1)
    return -1;
  
  ptr = tcc_get_symbol(s, "f");
  if (!ptr)
    return -1;
  result = callback(ptr);
  
  tcc_delete(s);
  
  return result;
}

/*
 * reg_pack_test: return a small struct which should be packed into
 * registers (Win32) during return.
 */
typedef struct reg_pack_test_type_s {int x, y;} reg_pack_test_type;
typedef reg_pack_test_type (*reg_pack_test_function_type) (reg_pack_test_type);

static int reg_pack_test_callback(void *ptr) {
  reg_pack_test_function_type f = (reg_pack_test_function_type)ptr;
  reg_pack_test_type a = {10, 35};
  reg_pack_test_type r;
  r = f(a);
  return ((r.x == a.x*5) && (r.y == a.y*3)) ? 0 : -1;
}

static int reg_pack_test(void) {
  const char *src =
  "typedef struct reg_pack_test_type_s {int x, y;} reg_pack_test_type;"
  "reg_pack_test_type f(reg_pack_test_type a) {\n"
  "  reg_pack_test_type r = {a.x*5, a.y*3};\n"
  "  return r;\n"
  "}\n";
  
  return run_callback(src, reg_pack_test_callback);
}

/*
 * sret_test: Create a struct large enough to be returned via sret
 * (hidden pointer as first function argument)
 */
typedef struct sret_test_type_s {
  long long a;
  long long b;
} sret_test_type;

typedef sret_test_type (*sret_test_function_type) (sret_test_type);

static int sret_test_callback(void *ptr) {
  sret_test_function_type f = (sret_test_function_type)(ptr);
  sret_test_type x = {5436LL, 658277698LL};
  sret_test_type r = f(x);
  return ((r.a==x.a*35)&&(r.b==x.b*19)) ? 0 : -1;
}

static int sret_test(void) {
  const char *src =
  "typedef struct sret_test_type_s {\n"
  "  long long a;\n"
  "  long long b;\n"
  "} sret_test_type;\n"
  "sret_test_type f(sret_test_type x) {\n"
  "  sret_test_type r = {x.a*35, x.b*19};\n"
  "  return r;\n"
  "}\n";
  
  return run_callback(src, sret_test_callback);
}

#define RUN_TEST(t) \
  do { \
    fputs(#t "... ", stdout); \
    fflush(stdout); \
    if (t() == 0) { \
      fputs("success\n", stdout); \
    } else { \
      fputs("failure\n", stdout); \
      retval = EXIT_FAILURE; \
    } \
  } while (0);

int main(int argc, char **argv) {
  int retval = EXIT_SUCCESS;
  /* if tcclib.h and libtcc1.a are not installed, where can we find them */
  if (argc == 2 && !memcmp(argv[1], "lib_path=",9))
    tccdir = argv[1] + 9;
  
  RUN_TEST(reg_pack_test);
  RUN_TEST(sret_test);
  return retval;
}
