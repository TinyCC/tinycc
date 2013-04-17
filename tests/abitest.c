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

typedef struct test1_type_s {int x, y;} test1_type;
typedef test1_type (*test1_function_type) ();

static int test1_callback(void *ptr) {
  test1_type r;
  r  = ((test1_function_type)ptr)();
  return ((r.x == 10) && (r.y == 35)) ? 0 : -1;
}

static int test1() {
  const char *src =
  "typedef struct test1_type_s {int x, y;} test1_type;"
  "test1_type f() {\n"
  "  test1_type r = {10, 35};\n"
  "  return r;\n"
  "}\n";
  
  return run_callback(src, test1_callback);
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
  
  RUN_TEST(test1);
  return retval;
}
