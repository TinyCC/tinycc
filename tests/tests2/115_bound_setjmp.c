#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#define TST   int i, a[2], b[2];                \
              for (i = 0; i < 2; i++) a[i] = 0; \
              for (i = 0; i < 2; i++) b[i] = 0

static jmp_buf jmp;

static void tst1 (void)
{
    TST;
    longjmp(jmp, 1);
}

static void tst2(void)
{
    jmp_buf jmp;

    setjmp (jmp);
    TST;
    tst1();
}

static void tst3 (jmp_buf loc)
{
    TST;
    longjmp(loc, 1);
}

static void tst4(jmp_buf loc)
{
    jmp_buf jmp;

    setjmp (jmp);
    TST;
    tst3(loc);
}

static void tst (void)
{
    jmp_buf loc;
    static int cnt;

    cnt = 0;
    if (setjmp (jmp) == 0) {
        TST;
        tst2();
    }
    else {
        cnt++;
    }
    if (setjmp (loc) == 0) {
        TST;
        tst4(loc);
    }
    else {
        cnt++;
    }
    if (cnt != 2)
        printf ("incorrect cnt %d\n", cnt);
}

static jmp_buf buf1;
static jmp_buf buf2;
static int *p;
static int n_x = 6;
static int g_counter;

static void stack (void)
{
    static int counter;
    static int way_point1;
    static int way_point2;

    counter = 0;
    way_point1 = 3;
    way_point2 = 2;
    g_counter = 0;
    if (setjmp (buf1) != 101) {
        int a[n_x];
        g_counter++;
        p = &a[0];
        if (g_counter < 5)
            longjmp (buf1, 2);
        else if (g_counter == 5)
            longjmp (buf1, 101);
        else {
            setjmp (buf2); 
            longjmp (buf1, 101);
        }
    }
    
    way_point1--;

    if (counter == 0) {
        counter++;
        {
            int a[n_x];
            g_counter++;
            p = &a[0];
            if (g_counter < 5)
                longjmp (buf1, 2);
            else if (g_counter == 5)
                longjmp (buf1, 101);
            else {
                setjmp (buf2);
                longjmp (buf1, 101);
            }
        }
    }

    way_point2--;

    if (counter == 1) {
        counter++;
        longjmp (buf2, 2);
    }

    if (!(way_point1 == 0 && way_point2 == 0 &&
          g_counter == 6 && counter == 2))
        printf ("Failed %d %d %d %d\n",
                way_point1, way_point2, g_counter, counter);
}

static jmp_buf env;
static int last_value;

static void jump (int val)
{
    longjmp (env, val);
}

static void check (void)
{
    int value;

    last_value = -1;
    value = setjmp (env);
    if (value != last_value + 1) {
        printf ("incorrect value %d %d\n",
                value, last_value + 1);
        return;
    }
    last_value = value;
    switch (value) {
#if !(defined(__FreeBSD__) || defined(__NetBSD__))
    /* longjmp(jmp_buf, 0) not supported */
    case 0:
        jump (0);
#endif
    default:
        if (value < 10)
          jump (value + 1);
    }
}

int
main (void)
{
    int i;

    for (i = 0; i < 10;  i++) {
        tst();
        stack();
        check();
    }
    return 0;
}


