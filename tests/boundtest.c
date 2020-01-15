#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NB_ITS 1000000
//#define NB_ITS 1
#define TAB_SIZE 100

int tab[TAB_SIZE];
int ret_sum;
char tab3[256];

int test1(void)
{
    int i, sum = 0;
    for(i=0;i<TAB_SIZE;i++) {
        sum += tab[i];
    }
    return sum;
}

/* error */
int test2(void)
{
    int i, sum = 0;
    for(i=0;i<TAB_SIZE + 1;i++) {
        sum += tab[i];
    }
    return sum;
}

/* actually, profiling test */
int test3(void)
{
    int sum;
    int i, it;

    sum = 0;
    for(it=0;it<NB_ITS;it++) {
        for(i=0;i<TAB_SIZE;i++) {
            sum += tab[i];
        }
    }
    return sum;
}

/* ok */
int test4(void)
{
    int i, sum = 0;
    int *tab4;

    fprintf(stderr, "%s start\n", __FUNCTION__);

    tab4 = malloc(20 * sizeof(int));
    for(i=0;i<20;i++) {
        sum += tab4[i];
    }
    free(tab4);

    fprintf(stderr, "%s end\n", __FUNCTION__);
    return sum;
}

/* error */
int test5(void)
{
    int i, sum = 0;
    int *tab4;

    fprintf(stderr, "%s start\n", __FUNCTION__);

    tab4 = malloc(20 * sizeof(int));
    for(i=0;i<21;i++) {
        sum += tab4[i];
    }
    free(tab4);

    fprintf(stderr, "%s end\n", __FUNCTION__);
    return sum;
}

/* error */
int test6(void)
{
    int i, sum = 0;
    int *tab4;
    
    tab4 = malloc(20 * sizeof(int));
    free(tab4);
    for(i=0;i<21;i++) {
        sum += tab4[i];
    }

    return sum;
}

/* error */
int test7(void)
{
    int i, sum = 0;
    int *p;

    for(i=0;i<TAB_SIZE + 1;i++) {
        p = &tab[i];
        if (i == TAB_SIZE)
            printf("i=%d %x\n", i, p);
        sum += *p;
    }
    return sum;
}

/* ok */
int test8(void)
{
    int i, sum = 0;
    int tab[10];

    for(i=0;i<10;i++) {
        sum += tab[i];
    }
    return sum;
}

/* error */
int test9(void)
{
    int i, sum = 0;
    char tab[10];

    for(i=0;i<11;i++) {
        sum += tab[i];
    }
    return sum;
}

/* ok */
int test10(void)
{
    char tab[10];
    char tab1[10];

    memset(tab, 0, 10);
    memcpy(tab, tab1, 10);
    memmove(tab, tab1, 10);
    return 0;
}

/* error */
int test11(void)
{
    char tab[10];

    memset(tab, 0, 11);
    return 0;
}

/* error */
int test12(void)
{
    void *ptr;
    ptr = malloc(10);
    free(ptr);
    free(ptr);
    return 0;
}

/* error */
int test13(void)
{
    char pad1 = 0;
    char tab[10];
    char pad2 = 0;
    memset(tab, 'a', sizeof(tab));
    return strlen(tab);
}

int test14(void)
{
    char *p = alloca(TAB_SIZE);
    memset(p, 'a', TAB_SIZE);
    p[TAB_SIZE-1] = 0;
    return strlen(p);
}

/* error */
int test15(void)
{
    char *p = alloca(TAB_SIZE-1);
    memset(p, 'a', TAB_SIZE);
    p[TAB_SIZE-1] = 0;
    return strlen(p);
}

/* ok */
int test16()
{
    char *demo = "This is only a test.";
    char *p;

    fprintf(stderr, "%s start\n", __FUNCTION__);

    p = alloca(16);
    strcpy(p,"12345678901234");
    printf("alloca: p is %s\n", p);

    /* Test alloca embedded in a larger expression */
    printf("alloca: %s\n", strcpy(alloca(strlen(demo)+1),demo) );

    fprintf(stderr, "%s end\n", __FUNCTION__);
    return 0;
}

/* error */
int test17()
{
    char *demo = "This is only a test.";
    char *p;

    fprintf(stderr, "%s start\n", __FUNCTION__);

    p = alloca(16);
    strcpy(p,"12345678901234");
    printf("alloca: p is %s\n", p);

    /* Test alloca embedded in a larger expression */
    printf("alloca: %s\n", strcpy(alloca(strlen(demo)),demo) );

    fprintf(stderr, "%s end\n", __FUNCTION__);
    return 0;
}

#define	CHECK(s)	if (strstr (__bound_error_msg, s) == 0) abort();

extern void __bound_never_fatal (int neverfatal);
extern const char *__bound_error_msg;

static void init18(char *a, char *b)
{
    memset (a, 'a', 10);
    a[3] = 0;
    a[9] = 0;
    memset (b, 'b', 10);
    __bound_error_msg = "";
}

/* ok (catch all errors) */
int test18()
{
    char pad1[10];
    char a[10];
    char pad2[10];
    char b[10];
    char pad3[10];

    memset (pad1, 0, sizeof(pad1));
    memset (pad2, 0, sizeof(pad2));
    memset (pad3, 0, sizeof(pad3));

    /* -2 in case TCC_BOUNDS_NEVER_FATAL is set */
    __bound_never_fatal (-2);

    /* memcpy */
    init18(a,b);
    memcpy(&a[1],&b[0],10);
    CHECK("memcpy dest");
    init18(a,b);
    memcpy(&a[0],&b[1],10);
    CHECK("memcpy src");
    init18(a,b);
    memcpy(&a[0],&a[3],4);
    CHECK("overlapping regions");
    init18(a,b);
    memcpy(&a[3],&a[0],4);
    CHECK("overlapping regions");

    /* memcmp */
    init18(a,b);
    memcmp(&b[1],&b[0],10);
    CHECK("memcmp s1");
    init18(a,b);
    memcmp(&b[0],&b[1],10);
    CHECK("memcmp s2");

    /* memmove */
    init18(a,b);
    memmove(&b[1],&b[0],10);
    CHECK("memmove dest");
    init18(a,b);
    memmove(&b[0],&b[1],10);
    CHECK("memmove src");

    /* memset */
    init18(a,b);
    memset(&b[1],'b',10);
    CHECK("memset");

    /* strlen */
    init18(a,b);
    strlen(&b[0]);
    CHECK("strlen");

    /* strcpy */
    init18(a,b);
    strcpy(&a[7], &a[0]);
    CHECK("strcpy dest");
    init18(a,b);
    strcpy(&a[0], &b[7]);
    CHECK("strcpy src");
    init18(a,b);
    strcpy(&a[0], &a[1]);
    CHECK("overlapping regions");
    init18(a,b);
    strcpy(&a[2], &a[0]);
    CHECK("overlapping regions");

    /* strncpy */
    init18(a,b);
    strncpy(&a[7], &a[0], 10);
    CHECK("strncpy dest");
    init18(a,b);
    strncpy(&a[0], &b[7], 10);
    CHECK("strncpy src");
    init18(a,b);
    strncpy(&a[0], &a[1], 10);
    CHECK("overlapping regions");
    strncpy(&a[2], &a[0], 10);
    CHECK("overlapping regions");

    /* strcmp */
    init18(a,b);
    strcmp(&b[2], &b[0]);
    CHECK("strcmp s1");
    init18(a,b);
    strcmp(&b[0], &b[2]);
    CHECK("strcmp s2");

    /* strncmp */
    init18(a,b);
    strncmp(&b[5], &b[0], 10);
    CHECK("strncmp s1");
    init18(a,b);
    strncmp(&b[0], &b[5], 10);
    CHECK("strncmp s2");

    /* strcat */
    init18(a,b);
    strcat(&a[7], &a[0]);
    CHECK("strcat dest");
    init18(a,b);
    strcat(&a[0], &b[5]);
    CHECK("strcat src");
    init18(a,b);
    strcat(&a[0], &a[4]);
    CHECK("overlapping regions");
    init18(a,b);
    strcat(&a[3], &a[0]);
    CHECK("overlapping regions");

    /* strchr */
    init18(a,b);
    strchr(&b[0], 'a');
    CHECK("strchr");

    /* strdup */
    init18(a,b);
    free(strdup(&b[0]));
    CHECK("strdup");

    __bound_never_fatal (2);

    /* memcpy */
    init18(a,b);
    memcpy(&a[0],&b[0],10);
    init18(a,b);
    memcpy(&a[0],&a[3],3);
    init18(a,b);
    memcpy(&a[3],&a[0],3);

    /* memcmp */
    init18(a,b);
    memcmp(&b[0],&b[0],10);

    /* memmove */
    init18(a,b);
    memmove(&b[0],&b[5],5);
    init18(a,b);
    memmove(&b[5],&b[0],5);

    /* memset */
    init18(a,b);
    memset(&b[0],'b',10);

    /* strlen */
    init18(a,b);
    strlen (&a[0]);

    /* strcpy */
    init18(a,b);
    strcpy (&a[0], &a[7]);

    /* strncpy */
    init18(a,b);
    strncpy (&a[0], &a[7], 4);

    /* strcmp */
    init18(a,b);
    strcmp (&a[0], &a[4]);

    /* strncmp */
    init18(a,b);
    strncmp (&a[0], &a[4], 10);

    /* strcat */
    init18(a,b);
    strcat (&a[0], &a[7]);

    /* strchr */
    init18(a,b);
    strchr (&a[0], 0);

    /* strdup */
    init18(a,b);
    free (strdup (&a[0]));

    return 0;
}

int (*table_test[])(void) = {
    test1,
    test2,
    test3,
    test4,
    test5,
    test6,
    test7,
    test8,
    test9,
    test10,
    test11,
    test12,
    test13,
    test14,
    test15,
    test16,
    test17,
    test18,
};

int main(int argc, char **argv)
{
    int i;
    char *cp;
    int index;
    int (*ftest)(void);
    int index_max = sizeof(table_test)/sizeof(table_test[0]);

    /* check bounds checking main arg */
    for (i = 0; i < argc; i++) {
        cp = argv[i];
        while (*cp) {
            cp++;
        }
    }

    if (argc < 2) {
        printf(
    	    "test TCC bound checking system\n"
	    "usage: boundtest N\n"
            "  1 <= N <= %d\n", index_max);
        exit(1);
    }

    index = 0;
    if (argc >= 2)
        index = atoi(argv[1]) - 1;

    if ((index < 0) || (index >= index_max)) {
        printf("N is outside of the valid range (%d)\n", index);
        exit(2);
    }

    /* well, we also use bounds on this ! */
    ftest = table_test[index];
    ftest();

    return 0;
}

/*
 * without bound   0.77 s
 * with bounds    4.73
 */  
