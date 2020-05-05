#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

static volatile int run = 1;
static int dummy[10];
static sem_t sem;

static void
add (void)
{
    int i;

    for (i = 0; i < (sizeof(dummy)/sizeof(dummy[0])); i++) {
        dummy[i]++;
    }
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

/* See tcc-doc.info */
#ifdef __BOUNDS_CHECKING_ON
extern void __bound_checking (int no_check);
#define BOUNDS_CHECKING_OFF __bound_checking(1)
#define BOUNDS_CHECKING_ON  __bound_checking(-1)
#else
#define BOUNDS_CHECKING_OFF
#define BOUNDS_CHECKING_ON
#endif

static void real_signal_handler(int sig)
{
    add();
    sem_post (&sem);
}

static void signal_handler(int sig)
{
    BOUNDS_CHECKING_OFF;
    real_signal_handler(sig);
    BOUNDS_CHECKING_ON;
}

int
main (void)
{
    int i;
    pthread_t id1, id2;
    struct sigaction act;
    struct timespec request;

    memset (&act, 0, sizeof (act));
    act.sa_handler = signal_handler;
    act.sa_flags = 0;
    sigemptyset (&act.sa_mask);
    sigaction (SIGUSR1, &act, NULL);
    sem_init (&sem, 1, 0);
    pthread_create(&id1, NULL, high_load, NULL);
    pthread_create(&id2, NULL, do_signal, NULL);
    clock_gettime (CLOCK_MONOTONIC, &request);
    request.tv_sec += 1;
    request.tv_nsec += 0;
    printf ("start\n");
    while (clock_nanosleep (CLOCK_MONOTONIC, TIMER_ABSTIME, &request, NULL)) {
    }
    printf ("end\n");
    run = 0;
    pthread_join(id1, NULL);
    pthread_join(id2, NULL);
    sem_destroy (&sem);
    return 0;
}
