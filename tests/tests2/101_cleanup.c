extern int printf(const char*, ...);
static int glob_i = 0;

void incr_glob_i(int *i)
{
  glob_i += *i;
}

#define INCR_GI {						\
    int i __attribute__ ((__cleanup__(incr_glob_i))) = 1;	\
  }

#define INCR_GI0 INCR_GI INCR_GI INCR_GI INCR_GI
#define INCR_GI1 INCR_GI0 INCR_GI0 INCR_GI0 INCR_GI0
#define INCR_GI2 INCR_GI1 INCR_GI1 INCR_GI1 INCR_GI1
#define INCR_GI3 INCR_GI2 INCR_GI2 INCR_GI2 INCR_GI2
#define INCR_GI4 INCR_GI3 INCR_GI3 INCR_GI3 INCR_GI3
#define INCR_GI5 INCR_GI4 INCR_GI4 INCR_GI4 INCR_GI4
#define INCR_GI6 INCR_GI5 INCR_GI5 INCR_GI5 INCR_GI5
#define INCR_GI7 INCR_GI6 INCR_GI6 INCR_GI6 INCR_GI6


void check2(char **hum);

void check(int *j)
{
    char * __attribute__ ((cleanup(check2))) stop_that = "wololo";
    int chk = 0;

    {
	char * __attribute__ ((cleanup(check2))) stop_that = "plop";

	{
	  non_plopage:
	    printf("---- %d\n", chk);
	}
	if (!chk) {
	    chk = 1;
	    goto non_plopage;
	}
    }

    {
	char * __attribute__ ((cleanup(check2))) stop_that = "tata !";

	goto out;
	stop_that = "titi";
    }
  again:
    chk = 2;
    {
	char * __attribute__ ((cleanup(check2))) cascade1 = "1";
	{
	    char * __attribute__ ((cleanup(check2))) cascade2 = "2";
	    {
		char * __attribute__ ((cleanup(check2))) cascade3 = "3";

		goto out;
		cascade3 = "nope";
	    }
	}
    }
  out:
    if (chk != 2)
	goto again;
    {
	{
	    char * __attribute__ ((cleanup(check2))) out = "last goto out";
	    ++chk;
	    if (chk != 3)
		goto out;
	}
    }
    return;
}

void check_oh_i(char *oh_i)
{
    printf("c: %c\n", *oh_i);
}

void goto_hell(double *f)
{
    printf("oo: %f\n", *f);
}

char *test()
{
    char *__attribute__ ((cleanup(check2))) str = "I don't think this should be print(but gcc got it wrong too)";

    return str;
}

void test_ret_subcall(char *that)
{
    printf("should be print before\n");
}

void test_ret()
{
    char *__attribute__ ((cleanup(check2))) that = "that";
    return test_ret_subcall(that);
}

void test_ret2()
{
  char *__attribute__ ((cleanup(check2))) that = "-that";
  {
    char *__attribute__ ((cleanup(check2))) that = "this should appear only once";
  }
  {
    char *__attribute__ ((cleanup(check2))) that = "-that2";
    return;
  }
}

void test2(void) {
    int chk = 0;
again:
    if (!chk) {
        char * __attribute__ ((cleanup(check2))) stop_that = "test2";
        chk++;
        goto again;
    }
}

int test3(void) {
    char * __attribute__ ((cleanup(check2))) stop_that = "three";
    int chk = 0;

    if (chk) {
        {
          outside:
	    {
            char * __attribute__ ((cleanup(check2))) stop_that = "two";
            printf("---- %d\n", chk);
	    }
        }
    }
    if (!chk)
    {
        char * __attribute__ ((cleanup(check2))) stop_that = "one";

        if (!chk) {
            chk = 1;
            goto outside;
        }
    }
    return 0;
}

void cl(int *ip)
{
    printf("%d\n", *ip);
}

void loop_cleanups(void)
{
    __attribute__((cleanup(cl))) int l = 1000;

    printf("-- loop 0 --\n");
    for ( __attribute__((cleanup(cl))) int i = 0; i < 10; ++i) {
        __attribute__((cleanup(cl))) int j = 100;
    }

    printf("-- loop 1 --\n");
    for (__attribute__((cleanup(cl))) int i = 0; i < 10; ++i) {
        __attribute__((cleanup(cl)))  int j = 200;
        continue;
    }

    printf("-- loop 2 --\n");
    for (__attribute__((cleanup(cl))) int i = 0; i < 10; ++i) {
        __attribute__((cleanup(cl))) int j = 300;
        break;
    }

    printf("-- loop 3 --\n");
    for (int i = 0; i < 2; ++i) {
	__attribute__((cleanup(cl))) int j = 400;
	switch (i) {
	case 0:
	    continue;
	default:
	{
	    __attribute__((cleanup(cl))) int jj = 500;
	    break;
	}
	}
    }
    printf("after break\n");
}

int main()
{
    int i __attribute__ ((__cleanup__(check))) = 0, not_i;
    int chk = 0;
    (void)not_i;

    {
	__attribute__ ((__cleanup__(check_oh_i))) char oh_i = 'o', o = 'a';
    }

    INCR_GI7;
    printf("glob_i: %d\n", glob_i);
 naaaaaaaa:
    if (!chk) {
	__attribute__ ((__cleanup__(check_oh_i))) char oh_i = 'f';
	double __attribute__ ((__cleanup__(goto_hell))) f = 2.6;

	chk = 1;
	goto naaaaaaaa;
    }
    i = 105;
    printf("because what if free was call inside cleanup function %s\n", test());
    test_ret();
    test_ret2();
    test2();
    test3();
    loop_cleanups();
    return i;
}

void check2(char **hum)
{
    printf("str: %s\n", *hum);
}
