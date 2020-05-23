#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#if !defined(_WIN32)
#include <pthread.h>
#else
#include <windows.h>
#endif

#define	SIZE	10
#define	COUNT	10

#define TST   int i, a[2], b[2];                \
              for (i = 0; i < 2; i++) a[i] = 0; \
              for (i = 0; i < 2; i++) b[i] = 0

static int count[SIZE];

static void tst1 (jmp_buf loc)
{
    TST;
    longjmp(loc, 1);
}

static void tst2(jmp_buf loc)
{
    jmp_buf jmp;

    setjmp (jmp);
    TST;
    tst1(loc);
}

static void *tst (void * index)
{
    jmp_buf loc;
    int i = *(int *) index;
    static int v[SIZE];

    for (v[i] = 0; v[i] < COUNT; v[i]++) {
        if (setjmp (loc) == 0) {
            TST;
            tst2(loc);
        }
        else {
            count[i]++;
        }
        i = *(int *) index;
    }
    return NULL;
}

int
main (void)
{
    int i;
#if !defined(_WIN32)
    pthread_t id[SIZE];
#else
    HANDLE id[SIZE];
#endif
    int index[SIZE];

    for (i = 0; i < SIZE; i++) {
        index[i] = i;
#if !defined(_WIN32)
        pthread_create (&id[i], NULL, tst, (void *) &index[i]);
#else
        id[i] = CreateThread(NULL, 8192, (LPTHREAD_START_ROUTINE) tst, (void *) &index[i], 0, NULL);
#endif
    }
    for (i = 0; i < SIZE; i++) {
#if !defined(_WIN32)
        pthread_join (id[i], NULL);
#else
        WaitForSingleObject(id[i], INFINITE);
#endif
    }
    for (i = 0; i < SIZE; i++) {
        if (count[i] != COUNT)
            printf ("error: %d %d\n", i, count[i]);
    }
    return 0;
}
