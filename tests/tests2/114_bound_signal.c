#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <setjmp.h>

/* See tcc-doc.info */
#if defined(__TINYC__) && __BOUNDS_CHECKING_ON
#undef __attribute__
extern void __bound_checking (int no_check);
#define BOUNDS_CHECKING_OFF __bound_checking(1)
#define BOUNDS_CHECKING_ON  __bound_checking(-1)
#define BOUNDS_NO_CHECKING __attribute__((bound_no_checking))
#else
#define BOUNDS_CHECKING_OFF
#define BOUNDS_CHECKING_ON
#define BOUNDS_NO_CHECKING
#endif

static volatile int run = 1;
static int dummy[10];
static sem_t sem;

static void
add (void) BOUNDS_NO_CHECKING
{
    int i;

    for (i = 0; i < (sizeof(dummy)/sizeof(dummy[0])); i++) {
        dummy[i]++;
    }
    /* Should not be translated into __bound_memset */
    memset (&dummy[0], 0, sizeof(dummy));
}

static void *
high_load (void *unused)
{
    while (run) {
        add();
    }
    return NULL;
}

static void *
do_signal (void *unused)
{
    while (run) {
        kill (getpid(), SIGUSR1);
        while (sem_wait(&sem) < 0 && errno == EINTR);
    }
    return NULL;
}

static void signal_handler(int sig) BOUNDS_NO_CHECKING
{
    add();
    sem_post (&sem);
}

int
main (void)
{
    int i;
    pthread_t id1, id2;
    struct sigaction act;
    sigjmp_buf sj;
    sigset_t m;
    time_t end;

    memset (&act, 0, sizeof (act));
    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    sigemptyset (&act.sa_mask);
    sigaction (SIGUSR1, &act, NULL);

    sem_init (&sem, 1, 0);
    pthread_create(&id1, NULL, high_load, NULL);
    pthread_create(&id2, NULL, do_signal, NULL);

    printf ("start\n");
    /* sleep does not work !!! */
    end = time(NULL) + 2;
    while (time(NULL) < end) ;
    run = 0;
    printf ("end\n");

    pthread_join(id1, NULL);
    pthread_join(id2, NULL);
    sem_destroy (&sem);

    sigemptyset (&m);
    sigprocmask (SIG_SETMASK, &m, NULL);
    if (sigsetjmp (sj, 0) == 0)
    {
        sigaddset (&m, SIGUSR1);
        sigprocmask (SIG_SETMASK, &m, NULL);
        siglongjmp (sj, 1);
        printf ("failed");
        return 1;
    }
    sigprocmask (SIG_SETMASK, NULL, &m);
    if (!sigismember (&m, SIGUSR1))
        printf ("failed");
    return 0;
}
